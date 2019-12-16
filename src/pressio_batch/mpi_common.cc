#include "mpi_common.h"
#include <algorithm>
#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>

MPI_Datatype work_request_t;
MPI_Datatype work_response_t;

void setup_types() {
  //create the request type
  {
  work_request r;
  int blockcounts[1] = {3};
  MPI_Aint displacements[1] = {0};
  MPI_Datatype types[1] = {MPI_INT};
  MPI_Type_create_struct(1, blockcounts, displacements, types, &work_request_t);
  MPI_Type_commit(&work_request_t);
  }

  //create the response type
  {
  work_response r;
  int blockcounts[2] = {3, 1};
  MPI_Aint displacements[2];
  MPI_Get_address(&r.task_id, &displacements[0]);
  MPI_Get_address(&r.value, &displacements[1]);
  displacements[1] -= displacements[0];
  displacements[0] = 0;
  MPI_Datatype types[2] = {MPI_INT, MPI_DOUBLE};
  MPI_Type_create_struct(2, blockcounts, displacements, types, &work_response_t);
  MPI_Type_commit(&work_response_t);
  }
}

void free_types() {
  MPI_Type_free(&work_request_t);
  MPI_Type_free(&work_response_t);
}

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
