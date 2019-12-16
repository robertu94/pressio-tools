#include <mpi.h>

#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>
#include <libpressio_ext/cpp/printers.h>
#include "mpi_common.h"
#include "mpi_master.h"
#include "mpi_worker.h"
#include "cmdline.h"
#include "datasets.h"
#include "compressor_configs.h"
#include "io.h"

int main(int argc, char *argv[])
{
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  auto cmdline = parse_args(argc, argv);
  setup_types();

  auto library = pressio_instance();
  auto metrics = pressio_new_metrics(library, cmdline.metrics.data(), cmdline.metrics.size());

  auto datasets = load_datasets(cmdline.datasets, rank == 0);
  auto compressors = load_compressors(cmdline.compressors, rank == 0);

  if(rank) worker(datasets, compressors, library, metrics, cmdline.fields);
  else master(datasets, compressors, cmdline.fields, metrics);

  free_types();
  MPI_Finalize();
  return 0;
}
