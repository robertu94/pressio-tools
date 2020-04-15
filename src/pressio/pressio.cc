#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <mpi.h>
#include <libpressio/libpressio.h>
#include <libpressio/libpressio_ext/io/pressio_io.h>
#include <libpressio/libpressio_ext/cpp/options.h>
#include <libpressio/libpressio_ext/cpp/printers.h>

#include <utils/string_options.h>
#include "cmdline.h"

static int rank = 0;

void print_versions(pressio* library) {
  if(rank == 0) {
    std::cerr << "libpressio version: " << pressio_version() << std::endl;
    std::istringstream compressors{std::string(pressio_supported_compressors())};
    std::string compressor;
    while(compressors >> compressor)
    {
      auto* cmp = pressio_get_compressor(library, compressor.c_str());
      std::cerr << compressor << ' ' << pressio_compressor_version(cmp) << std::endl;
    }
  }
}

template <class ForwardIt>
void print_selected_options(pressio_options* options, ForwardIt begin, ForwardIt end) {
  if(rank == 0) {
  std::for_each(
      begin,
      end,
      [options](std::string const option){
        if (option == "all") {
          std::cerr << (*options);
        } else { 
          try {
            std::cerr << option << options->get(option) << std::endl;
          } catch(std::out_of_range const&) {
            std::cerr << ": option is unknown" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
      });
  }
}


std::tuple<pressio_compressor*, pressio_metrics*, pressio_options*> setup_compressor(struct pressio* pressio, options opts) {
  auto* compressor = pressio_get_compressor(pressio, opts.compressor.c_str());
  if (compressor == nullptr) {
    if(rank == 0) {
    std::cerr << "failed to initialize the compressor: " << compressor << std::endl;
    std::cerr << pressio_error_msg(pressio) << std::endl;
    }
    exit(pressio_error_code(pressio));
  }

  auto* metrics = pressio_new_metrics(pressio, opts.metrics_ids.data(), opts.metrics_ids.size());
  if (metrics == nullptr) {
    if(rank == 0) {
    std::cerr << "failed to initialize the metrics" << std::endl;
    std::cerr << pressio_error_msg(pressio) << std::endl;
    }
    exit(pressio_error_code(pressio));
  }
  pressio_options* metrics_options = pressio_metrics_get_options(metrics);
  auto metric_opts = options_from_multimap(opts.metrics_options);
  for(auto const& [setting, value]: *metric_opts) {
    auto status = pressio_options_cast_set(metrics_options,
        setting.c_str(),
        &value,
        pressio_conversion_special
        );
    switch(status) {
      case pressio_options_key_does_not_exist:
        {
          if(rank == 0) {
            std::cerr << "non existent option for the metric: " << setting  << std::endl;
          }
          exit(EXIT_FAILURE);
        }
      case pressio_options_key_exists:
        {
          if(rank == 0) {
            std::cerr << "cannot convert to correct type: " << value << " for setting " << setting  << std::endl;
          }
          exit(EXIT_FAILURE);
        }
      default:
        (void)0;
    }
  }
  pressio_metrics_set_options(metrics, metrics_options);
  pressio_compressor_set_metrics(compressor, metrics);
  pressio_options_free(metric_opts);

  pressio_options* early_compressor_options = options_from_multimap(opts.early_options);
  if (pressio_compressor_check_options(compressor, early_compressor_options)) {
    if(rank == 0) {
      std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    }
    exit(pressio_compressor_error_code(compressor));
  }
  if (pressio_compressor_set_options(compressor, early_compressor_options)) {
    if(rank == 0) {
      std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    }
    exit(pressio_compressor_error_code(compressor));
  }

  auto* compressor_options = pressio_compressor_get_options(compressor);
  auto pressio_opts = options_from_multimap(opts.options);
  for(auto const& [setting, value]: *pressio_opts) {
    auto status = pressio_options_cast_set(compressor_options,
        setting.c_str(),
        &value,
        pressio_conversion_special
        );
    switch(status) {
      case pressio_options_key_does_not_exist:
        if(rank == 0) {
          std::cerr << "non existent option for the compressor: " << setting  << std::endl;
        }
        exit(EXIT_FAILURE);
      case pressio_options_key_exists:
        if(rank == 0) {
          std::cerr << "cannot convert to correct type: " << value << " for setting " << setting  << std::endl;
        }
        exit(EXIT_FAILURE);
      default:
        (void)0;
    }
  }
  pressio_options_free(pressio_opts);
  
  if (pressio_compressor_check_options(compressor, compressor_options)) {
    if(rank == 0) {
      std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    }
    exit(pressio_compressor_error_code(compressor));
  }
  if (pressio_compressor_set_options(compressor, compressor_options)) {
    if(rank == 0) {
      std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    }
    exit(pressio_compressor_error_code(compressor));
  }
  pressio_options_free(compressor_options);
  //meta-compressors need their options called twice to get the complete list of options
  compressor_options = pressio_compressor_get_options(compressor);
    

  return {compressor, metrics, compressor_options};
}

pressio_data* compress(struct pressio_compressor* compressor, options const& opts) {

  auto* compressed = pressio_data_new_empty(pressio_byte_dtype, 0, nullptr);
  if (pressio_compressor_compress(compressor, opts.input, compressed)) {
    if(rank == 0) {
      std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    }
    exit(pressio_compressor_error_code(compressor));
  }
  return compressed;
}

pressio_data* decompress(struct pressio_compressor* compressor, pressio_data* compressed,  options const& opts) {


  auto* output_buffer = pressio_data_new_clone(opts.input);
  if (pressio_compressor_decompress(compressor, compressed, output_buffer)) {
    if(rank == 0) {
      std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    }
    exit(pressio_compressor_error_code(compressor));
  }
  return output_buffer;

}


int
main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  auto opts = parse_args(argc, argv);
  auto* pressio = pressio_instance();
  if(pressio == nullptr) {
    if(rank == 0) {
      std::cerr << "failed to initialize libpressio" << std::endl;
    }
    exit(EXIT_FAILURE);
  }

  if(opts.actions.find(Action::Version) != opts.actions.end()) {
    print_versions(pressio);
  }

  if (opts.actions.find(Action::Compress) != opts.actions.end() or
      opts.actions.find(Action::Decompress) != opts.actions.end() or
      opts.actions.find(Action::Settings) != opts.actions.end()
      ) {

    auto [compressor, metrics, options] = setup_compressor(pressio, opts);
    pressio_data* compressed = nullptr;
    pressio_data* decompressed = nullptr;

    if (opts.actions.find(Action::Settings) != opts.actions.end()) {
      print_selected_options(options, std::begin(opts.print_options), std::end(opts.print_options));

      pressio_options* configuration = pressio_compressor_get_configuration(compressor);
      print_selected_options(configuration, std::begin(opts.print_compile_options), std::end(opts.print_compile_options));
      pressio_options_free(configuration);

      pressio_options* metrics_options = pressio_compressor_metrics_get_options(compressor);
      print_selected_options(metrics_options, std::begin(opts.print_metrics_options), std::end(opts.print_metrics_options));
      pressio_options_free(metrics_options);

      pressio_options* input_options = pressio_io_get_options(opts.input_file_action);
      print_selected_options(input_options, std::begin(opts.print_io_input_options), std::end(opts.print_io_input_options));
      pressio_options_free(input_options);
      
      pressio_options* comp_options = pressio_io_get_options(opts.compressed_file_action);
      print_selected_options(comp_options, std::begin(opts.print_io_comp_options), std::end(opts.print_io_comp_options));
      pressio_options_free(comp_options);

      pressio_options* decomp_options = pressio_io_get_options(opts.decompressed_file_action);
      print_selected_options(decomp_options, std::begin(opts.print_io_decomp_options), std::end(opts.print_io_decomp_options));
      pressio_options_free(decomp_options);

    }
    
    if (opts.actions.find(Action::Compress) != opts.actions.end()) {
      compressed = compress(compressor, opts);
      if (auto result = pressio_io_write(opts.compressed_file_action,compressed)) {
        if(rank == 0) {
          std::cerr << "writing compressed file failed " << pressio_io_error_msg(opts.compressed_file_action) << std::endl;
        }
        exit(EXIT_FAILURE);
      }
    } else if (opts.actions.find(Action::Decompress) != opts.actions.end()) {
      std::vector<size_t> dims;
      for (size_t i = 0; i < pressio_data_num_dimensions(opts.input); ++i) {
        dims.push_back(pressio_data_get_dimension(opts.input, i));
      }
      compressed = pressio_data_new_nonowning(
          pressio_data_dtype(opts.input),
          pressio_data_ptr(opts.input,nullptr),
          dims.size(),
          dims.data()
          );
    }

    if (opts.actions.find(Action::Decompress) != opts.actions.end()) {
      decompressed = decompress(compressor, compressed, opts);
    }
    if (auto result = pressio_io_write(opts.decompressed_file_action,decompressed)) {
      if(rank == 0) {
        std::cerr << "writing decompressed file failed " << pressio_io_error_msg(opts.decompressed_file_action) << std::endl;
      }
      exit(EXIT_FAILURE);
    }

    auto* metrics_results = pressio_compressor_get_metrics_results(compressor);
    print_selected_options(metrics_results, std::begin(opts.print_metrics), std::end(opts.print_metrics));
    pressio_options_free(metrics_results);

    pressio_data_free(compressed);
    pressio_data_free(decompressed);
    pressio_metrics_free(metrics);
    pressio_options_free(options);
    pressio_compressor_release(compressor);
  }

  pressio_data_free(opts.input);

  pressio_release(pressio);
  MPI_Finalize();
  return 0;
}
