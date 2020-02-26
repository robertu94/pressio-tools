#include <iostream>
#include <mpi.h>

#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>
#include <libpressio_ext/io/posix.h>
#include <libdistributed_work_queue.h>

#include "cmdline.h"
#include "datasets.h"
#include "compressor_configs.h"
#include "io.h"

namespace queue = distributed::queue;
using RequestType = std::tuple<int,int,int>; //task_id, dataset_id, compressor_id
using ResponseType = std::tuple<int,int,double>; //task_id, metric_id, metric


std::map<int, std::string> init_fieldnames(std::vector<std::string> const& fields, pressio_metrics* metrics) {
  std::map<int, std::string> id_to_fieldname;
  int id = 0;

  if(fields.empty()) {
    pressio_options* fields_opts = pressio_metrics_get_results(metrics);
    std::vector<std::string> field_names;
    for (auto const& field : *fields_opts) {
      field_names.emplace_back(field.first);
    }
    std::sort(std::begin(field_names), std::end(field_names));
    for (auto const& field : field_names) {
      id_to_fieldname[id++] = field;
    }
    pressio_options_free(fields_opts);
  } else {
    for (auto const& field : fields) {
      id_to_fieldname[id++] = field;
    }
  }
  return id_to_fieldname;
}

int main(int argc, char *argv[])
{
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  auto cmdline = parse_args(argc, argv, rank == 0);
  if(cmdline.error_code) {
    MPI_Finalize();
    exit(1);
  }

  auto library = pressio_instance();
  auto metrics = pressio_new_metrics(library, cmdline.metrics.data(), cmdline.metrics.size());
  auto datasets = load_datasets(cmdline.datasets, rank == 0);
  auto compressors = load_compressors(cmdline.compressors, rank == 0);

  //create the list of tasks
  std::vector<RequestType> tasks;
  std::map<int, std::string> task_to_name;
  int task_id = 0;
  for (int replicant_id = 0; replicant_id < cmdline.replicats; ++replicant_id) {
    for (int dataset_id = 0; dataset_id < datasets.size(); ++dataset_id) {
      for (int compressor_id = 0; compressor_id < compressors.size(); ++compressor_id) {
        task_to_name[task_id] = datasets[dataset_id]->get_name() + \
                                compressors[compressor_id]->get_name();
        tasks.emplace_back(task_id++, dataset_id, compressor_id);
      }
    }
  }
  std::map<int, std::string> id_to_fieldname = init_fieldnames(cmdline.fields, metrics);
  if(cmdline.fields.empty())
  {
    std::transform(std::begin(id_to_fieldname),
                   std::end(id_to_fieldname),
                   std::back_inserter(cmdline.fields),
                   [](auto const& field) {return field.second;}
                  );
  }

  std::map<std::string, int> fieldname_to_id;
  for (auto const& field : id_to_fieldname) {
    fieldname_to_id[field.second] = field.first;
  }

  //output the header
  if(rank == 0) {
    std::cout << "configuration";
    for (auto const& field : cmdline.fields) {
      std::cout << ',' << field;
    }
    std::cout << std::endl;
  }

  //prepare the receive responses
  std::map<int,pressio_options> responses;
  std::map<int,int> response_count;

  //do the work
  queue::work_queue(
      MPI_COMM_WORLD,
      std::begin(tasks), std::end(tasks),
      [&](RequestType request) {
        auto [task_id, dataset_id, compressor_id] = request;
        std::vector<ResponseType> task_responses;
        auto compressor = compressors[compressor_id]->load(library);
        auto input_data = datasets[dataset_id]->load();
        auto compressed = pressio_data_new_empty(pressio_byte_dtype, 0, nullptr);
        auto decompressed = pressio_data_new_clone(input_data);

        pressio_compressor_set_metrics(compressor, metrics);
        pressio_compressor_compress(compressor, input_data, compressed);
        pressio_compressor_decompress(compressor, compressed, decompressed);

        if(not cmdline.compressed_dir.empty()) {
          auto compressed_path = cmdline.compressed_dir + "/" + task_to_name[task_id];
          pressio_io_data_path_write(compressed, compressed_path.c_str());
        }
        if(not cmdline.decompressed_dir.empty()) {
          auto decompressed_path = cmdline.decompressed_dir + "/" + task_to_name[task_id];
          pressio_io_data_path_write(decompressed, decompressed_path.c_str());
        }

        auto metrics_results = pressio_compressor_get_metrics_results(compressor);
        for (auto metric_result : *metrics_results) {
          task_responses.emplace_back(
              /*task_id*/task_id,
              /*metric_id*/fieldname_to_id[metric_result.first],
              /*metric*/metric_result.second.as(pressio_option_double_type, pressio_conversion_explicit).get_value<double>()
            );
        }

        pressio_data_free(input_data);
        pressio_data_free(decompressed);
        pressio_data_free(compressed);
        pressio_compressor_release(compressor);
        pressio_options_free(metrics_results);
        return task_responses;
      },
      [&](std::vector<ResponseType> const& responses_v){
        int my_task_id;
        for (auto const& response : responses_v) {
          auto& [task_id, metric_id, metric] = response;
          my_task_id = task_id;
          responses[task_id].set(id_to_fieldname[metric_id], metric);
        }
        output_csv(
            std::cout,
            task_to_name[my_task_id],
            &responses[my_task_id],
            cmdline.fields
            );
      }
      );


  MPI_Finalize();
  return 0;
}
