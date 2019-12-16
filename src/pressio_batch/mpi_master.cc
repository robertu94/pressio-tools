#include "mpi_master.h"

#include <queue>
#include <map>
#include <libpressio_ext/cpp/options.h>

#include "mpi_common.h"
#include "io.h"

struct task {
  int compressor_id;
  int dataset_id;
};

std::queue<int> make_worker_queue() {
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  std::queue<int> workers;
  for (int i = 1; i < size; ++i) {
    workers.push(i);
  }
  return workers;
}

std::queue<task> make_tasks(const std::vector<std::unique_ptr<dataset>> &datasets,
                            const std::vector<std::unique_ptr<compressor_config>> &compressors) {
  std::queue<task> tasks;
  for (int compressor_id = 0; compressor_id < compressors.size(); ++compressor_id) {
    for (int dataset_id = 0; dataset_id < datasets.size(); ++dataset_id) {
      tasks.emplace(task{compressor_id, dataset_id});
    }
  }
  return tasks;
}
void launch_task(const std::vector<std::unique_ptr<dataset>> &datasets,
                 const std::vector<std::unique_ptr<compressor_config>> &compressors,
                 std::queue<int> &workers,
                 std::queue<task> &tasks,
                 int& task_id,
                 std::map<int, std::string> &task_to_name) {
  int worker = workers.front();
  workers.pop();

  work_request request;
  auto task = tasks.front();
  tasks.pop();
  request.task_id = task_id++;
  request.compressor_id = task.compressor_id;
  request.dataset_id = task.dataset_id;
  task_to_name[request.task_id] = datasets[task.dataset_id]->get_name() + compressors[task.compressor_id]->get_name();
  MPI_Send(&request, 1, work_request_t, worker, 0, MPI_COMM_WORLD);
}

void print_response(const std::vector<std::string> &fields,
                    bool& first,
                    const std::map<int, pressio_options> &results,
                    const std::map<int, std::string> &task_to_name,
                    const work_response &response) {
  if(first) {
    first=false;
    std::cout << "configuration";
    for (auto const& field : fields) {
      std::cout << ',' << field;
    }
    std::cout << std::endl;
  }
  output_csv(std::cout,
      task_to_name.find(response.task_id)->second,
      &results.find(response.task_id)->second,
      fields
      );
}

void terminate_workers() {
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  for (int i = 0; i < size; ++i) {
    work_request request;
    request.task_id = 0;
    MPI_Send(&request, 1, work_request_t, i, 0, MPI_COMM_WORLD);
  }
}

void master(std::vector<std::unique_ptr<dataset>> const& datasets,
            std::vector<std::unique_ptr<compressor_config>> const& compressors,
            std::vector<std::string> fields,
            pressio_metrics* metrics
            ) {
  std::map<int, std::string> id_to_fieldname = init_fieldnames(fields, metrics);
  std::transform(std::begin(id_to_fieldname), std::end(id_to_fieldname), std::back_inserter(fields),
      [](auto const& field) {return field.second;});

  //create a queue of worker ids
  auto workers = make_worker_queue();
  auto tasks = make_tasks(datasets, compressors);

  int task_id = 1;
  bool first = true;
  std::map<int, pressio_options> results;
  std::map<int, std::string> task_to_name;

  while(!tasks.empty()) {
    //try to send tasks to workers
    while(!workers.empty() and !tasks.empty())
    {
      launch_task(datasets, compressors, workers, tasks, task_id, task_to_name);
    }

    bool worker_ready = false;
    while(!worker_ready)
    {
      work_response response;
      MPI_Status status;
      MPI_Recv(&response, 1 , work_response_t, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(response.done) {
        print_response(fields, first, results, task_to_name, response);
        workers.push(status.MPI_SOURCE);
        worker_ready = true;
      } else {
        results[response.task_id].set(id_to_fieldname[response.field_id], response.value);
      }
    }

  }

  //all done send all workers a done signal
  terminate_workers();
}
