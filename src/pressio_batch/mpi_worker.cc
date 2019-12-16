#include "mpi_worker.h"
#include "mpi_common.h"
#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>

pressio_options *run_compressor(const std::vector<std::unique_ptr<dataset>> &datasets,
                                pressio_compressor* compressor,
                                pressio_metrics *metrics,
                                const work_request &request) {
  pressio_options *metrics_results;
  auto input_data = datasets[request.dataset_id]->load();
  auto compressed = pressio_data_new_empty(pressio_byte_dtype, 0, nullptr);
  auto decompressed = pressio_data_new_clone(input_data);

  pressio_compressor_set_metrics(compressor, metrics);
  pressio_compressor_compress(compressor, input_data, compressed);
  pressio_compressor_decompress(compressor, compressed, decompressed);
  metrics_results = pressio_compressor_get_metrics_results(compressor);

  pressio_data_free(input_data);
  pressio_data_free(decompressed);
  pressio_data_free(compressed);
  pressio_compressor_release(compressor);
  return metrics_results;
}
void worker(std::vector<std::unique_ptr<dataset>> const& datasets, std::vector<std::unique_ptr<compressor_config>> const& compressors, pressio* library, pressio_metrics* metrics, std::vector<std::string> const& fields) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int stop_flag = false;
  work_request request;
  work_response response;

  std::map<int, std::string> id_to_fieldname = init_fieldnames(fields, metrics);
  std::map<std::string, int> fieldname_to_id;
  for (auto const& field : id_to_fieldname) {
    fieldname_to_id[field.second] = field.first;
  }

  while(!stop_flag) {
    MPI_Recv(&request, 1, work_request_t, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (request.task_id != TASK_DONE) {
      pressio_options* metrics_results;
      auto compressor = compressors[request.compressor_id]->load(library);
      metrics_results = run_compressor(datasets, compressor, metrics, request);

      for (auto metric_result : *metrics_results) {
        response.task_id = request.task_id;
        response.done = 0;
        response.field_id = fieldname_to_id[metric_result.first];
        response.value = metric_result.second.as(pressio_option_double_type, pressio_conversion_explicit).get_value<double>();
        MPI_Send(&response, 1, work_response_t, ROOT, 0, MPI_COMM_WORLD);
      }


      response.task_id = request.task_id;
      response.done = 1;
      response.field_id = 0;
      response.value = 0;
      MPI_Send(&response, 1, work_response_t, ROOT, 0, MPI_COMM_WORLD);
    } else {
      stop_flag = true;
    }
  }
}

