#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <libpressio/libpressio.h>
#include <libpressio/libpressio_ext/cpp/options.h>
#include <libpressio/libpressio_ext/cpp/printers.h>

#include "cmdline.h"

void print_versions(pressio* library) {
  std::cerr << "libpressio version: " << pressio_version() << std::endl;
  std::istringstream compressors{std::string(pressio_supported_compressors())};
  std::string compressor;
  while(compressors >> compressor)
  {
    auto* cmp = pressio_get_compressor(library, compressor.c_str());
    std::cerr << compressor << ' ' << pressio_compressor_version(cmp) << std::endl;
  }
}

std::tuple<pressio_compressor*, pressio_metrics*, pressio_options*> setup_compressor(struct pressio* pressio, options opts) {
  auto* compressor = pressio_get_compressor(pressio, opts.compressor.c_str());
  if (compressor == nullptr) {
    std::cerr << "failed to initialize the compressor: " << compressor << std::endl;
    std::cerr << pressio_error_msg(pressio) << std::endl;
    exit(pressio_error_code(pressio));
  }

  auto* metrics = pressio_new_metrics(pressio, opts.metrics_ids.data(), opts.metrics_ids.size());
  if (metrics == nullptr) {
    std::cerr << "failed to initialize the metrics" << std::endl;
    std::cerr << pressio_error_msg(pressio) << std::endl;
    exit(pressio_error_code(pressio));
  }
  pressio_compressor_set_metrics(compressor, metrics);


  auto* compressor_options = pressio_compressor_get_options(compressor);
  for(auto const& [setting, value]: opts.options) {
    auto* value_op = pressio_option_new_string(value.c_str());
    auto status = pressio_options_cast_set(compressor_options,
        setting.c_str(),
        value_op,
        pressio_conversion_special
        );
    switch(status) {
      case pressio_options_key_does_not_exist:
        std::cerr << "non existent option for the compressor: " << setting  << std::endl;
        exit(EXIT_FAILURE);
      case pressio_options_key_exists:
        std::cerr << "cannot convert to correct type: " << value << " for setting " << setting  << std::endl;
        exit(EXIT_FAILURE);
      default:
        (void)0;
    }
    pressio_option_free(value_op);
  }
  
  if (pressio_compressor_check_options(compressor, compressor_options)) {
    std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    exit(pressio_compressor_error_code(compressor));
  }
  if (pressio_compressor_set_options(compressor, compressor_options)) {
    std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    exit(pressio_compressor_error_code(compressor));
  }

  return {compressor, metrics, compressor_options};
}

pressio_data* compress(struct pressio_compressor* compressor, options const& opts) {

  auto* compressed = pressio_data_new_empty(pressio_byte_dtype, 0, nullptr);
  if (pressio_compressor_compress(compressor, opts.input, compressed)) {
    std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    exit(pressio_compressor_error_code(compressor));
  }
  return compressed;
}

pressio_data* decompress(struct pressio_compressor* compressor, pressio_data* compressed,  options const& opts) {


  auto* output_buffer = pressio_data_new_clone(opts.input);
  if (pressio_compressor_decompress(compressor, compressed, output_buffer)) {
    std::cerr << pressio_compressor_error_msg(compressor) << std::endl;
    exit(pressio_compressor_error_code(compressor));
  }
  return output_buffer;

}


template <class ForwardIt>
void print_selected_options(pressio_options* options, ForwardIt begin, ForwardIt end) {
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

int
main(int argc, char* argv[])
{
  auto opts = parse_args(argc, argv);
  auto* pressio = pressio_instance();
  if(pressio == nullptr) {
    std::cerr << "failed to initialize libpressio" << std::endl;
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
    }
    
    if (opts.actions.find(Action::Compress) != opts.actions.end()) {
      compressed = compress(compressor, opts);
      if (auto result = opts.compressed_file_action(compressed)) {
        std::cerr << result.err_msg << std::endl;
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
    if (auto result = opts.decompressed_file_action(decompressed)) {
      std::cerr << result.err_msg << std::endl;
      exit(EXIT_FAILURE);
    }

    auto* metrics_results = pressio_compressor_get_metrics_results(compressor);
    print_selected_options(metrics_results, std::begin(opts.print_metrics), std::end(opts.print_metrics));
    pressio_options_free(metrics_results);

    pressio_data_free(compressed);
    pressio_data_free(decompressed);
    pressio_metrics_free(metrics);
    pressio_options_free(options);
  }

  pressio_data_free(opts.input);


  return 0;
}
