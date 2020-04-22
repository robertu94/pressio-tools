#include <iostream>
#include <mpi.h>
#include <vector>
#include <libpressio_ext/cpp/libpressio.h>
#include <pressio_search_defines.h>
#include <libpressio_ext/io/pressio_io.h>


double run_lossless(pressio_options* opt_options, pressio_metrics* metrics, pressio_data* input) {
  pressio library;
  pressio_compressor noop = library.get_compressor("noop");
  noop->set_metrics(*metrics);
  pressio_data compressed = pressio_data::empty(pressio_byte_dtype, {});
  pressio_data decompressed = pressio_data::owning(input->dtype(), input->dimensions());

  noop->compress(input, &compressed);
  noop->decompress(&compressed, &decompressed);
  pressio_options metrics_results = noop->get_metrics_results();

  std::vector<std::string> opt_outputs;
  opt_options->get("opt:output", &opt_outputs);
  std::vector<double> opt_values;


  return 0;
}

double find_target(pressio_compressor* compressor, pressio_data* input, double target) {

  double d = 0.0;
  return d;
}

int main(int argc, char *argv[])
{
  int rank, size, thread_provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &thread_provided);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(thread_provided != MPI_THREAD_MULTIPLE) {
    if(rank == 0){
      std::cout << "insufficient thread support from MPI" << std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD, 3);
  }

  pressio library;
  std::string metrics_ids[] = {"size", "time", "error_stat"};
  pressio_metrics metrics = library.get_metrics(std::begin(metrics_ids), std::end(metrics_ids));
  pressio_compressor compressor = library.get_compressor("opt");
  auto configuration = compressor->get_configuration();
  compressor->set_metrics(metrics);
  if(rank == 0) {
    std::cout << "configuration:" << std::endl << configuration << std::endl;
  }

  //generic input
  const char* input_format = "posix";
  auto input_io = library.get_io(input_format);
  pressio_dtype dtype = pressio_byte_dtype;
  std::vector<size_t> sizes;
  auto input_desc = pressio_data::empty(dtype, sizes);
  auto input = input_io->read(&input_desc);

  pressio_data lower_bound{0.0};
  pressio_data upper_bound{0.1};
  double psnr_threshold = 65.0;
  std::function<double(std::vector<double>const&)> objective = [=](std::vector<double> const& results) {
    double cr = results.at(0);
    double psnr = results.at(1);
    if(psnr < psnr_threshold) return std::numeric_limits<double>::lowest();
    return cr;
  };


  pressio_options opt_options;
  opt_options.set("opt:search", "dist_gridsearch"); //binary search is non-monotonic for this input using SZ_REL
  opt_options.set("dist_gridsearch:search", "fraz"); //binary search is non-monotonic for this input using SZ_REL
  opt_options.set("dist_gridsearch:num_bins", pressio_data{5ul,});
  opt_options.set("dist_gridsearch:overlap_percentage", pressio_data{.1,});
  opt_options.set("dist_gridsearch:comm", (void*)MPI_COMM_WORLD);
  opt_options.set("fraz:nthreads", 4u);
  opt_options.set("opt:compressor", "sz");
  opt_options.set("opt:inputs", std::vector<std::string>{"sz:rel_err_bound"});
  opt_options.set("opt:lower_bound", lower_bound);
  opt_options.set("opt:upper_bound", upper_bound);
  opt_options.set("opt:target", 400.0);
  opt_options.set("opt:local_rel_tolerance", 0.1);
  opt_options.set("opt:global_rel_tolerance", 0.1);
  opt_options.set("opt:max_iterations", 100u);
  opt_options.set("opt:output", std::vector<std::string>{"size:compression_ratio", "error_stat:psnr"});
  opt_options.set("opt:do_decompress", 0);
  opt_options.set("opt:search_metrics", "progress_printer");
  opt_options.set("opt:do_decompress", 1);
  opt_options.set("sz:error_bound_mode_str", "REL");

  auto lossless = run_lossless(&opt_options, &metrics, input);
  auto near_lossless = find_target(&compressor, input, lossless * .98);
  auto garbage = find_target(&compressor, input, lossless * .30);



  MPI_Finalize();
  return 0;
}
