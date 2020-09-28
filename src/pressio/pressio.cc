#include <iostream>
#include <libpressio_ext/cpp/configurable.h>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <mpi.h>
#include <libpressio/libpressio.h>
#include <libpressio/libpressio_ext/io/pressio_io.h>
#include <libpressio/libpressio_ext/cpp/io.h>
#include <libpressio/libpressio_ext/cpp/options.h>
#include <libpressio/libpressio_ext/cpp/printers.h>
#include <libpressio/libpressio_ext/cpp/compressor.h>
#include <libpressio/libpressio_ext/cpp/metrics.h>
#include <libpressio/libpressio_ext/cpp/pressio.h>
#include <libpressio/libpressio_ext/cpp/serializable.h>
#include <libdistributed_comm.h>

#include <utils/string_options.h>
#include "cmdline.h"

static int rank = 0;

void print_versions(pressio& library) {
  if(rank == 0) {
    std::cerr << "libpressio version: " << pressio_version() << std::endl;
    std::istringstream compressors{std::string(pressio_supported_compressors())};
    std::string compressor;
    while(compressors >> compressor)
    {
      auto cmp = library.get_compressor(compressor);
      std::cerr << compressor << ' ' << cmp->version() << std::endl;
    }
  }
}

template <class ForwardIt>
void print_selected_options(pressio_options const& options, ForwardIt begin, ForwardIt end) {
  if(rank == 0) {
  std::for_each(
      begin,
      end,
      [options](std::string const option){
        if (option == "all") {
          std::cerr << (options);
        } else { 
          try {
            std::cerr << option << options.get(option) << std::endl;
          } catch(std::out_of_range const&) {
            std::cerr << ": option is unknown" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
      });
  }
}

template <class MultiMap>
void set_options_from_multimap(pressio_configurable& c, MultiMap const& user_options, const char* configurable_type) {
  pressio_options configurable_options = c.get_options();
  pressio_options new_options;
  auto presssio_user_opts = options_from_multimap(user_options);
  for (auto const& [setting, value] : presssio_user_opts) {
    new_options.set(setting, configurable_options.get(setting));
    auto status = new_options.cast_set(setting, value, pressio_conversion_special);
    switch(status) {
      case pressio_options_key_does_not_exist:
        {
          if(rank == 0) {
            std::cerr << "non existent option for the " << configurable_type << " : " << setting  << std::endl;
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
  c.set_options(new_options);
}

template <class PressioObject, class MultiMap>
void set_early_options(PressioObject& c, MultiMap const& user_options) {
  pressio_options early_compressor_options = options_from_multimap(user_options);
  if (c.check_options(early_compressor_options)) {
    if(rank == 0) {
      std::cerr << c.error_msg() << std::endl;
    }
    exit(c.error_code());
  }
  if (c.set_options(early_compressor_options)) {
    if(rank == 0) {
      std::cerr << c.error_msg() << std::endl;
    }
    exit(c.error_code());
  }
}


pressio_compressor setup_compressor(pressio& library, cmdline_options const& opts) {
  pressio_compressor compressor = library.get_compressor(opts.compressor);
  if (!compressor) {
    if(rank == 0) {
    std::cerr << "failed to initialize the compressor: " << opts.compressor << std::endl;
    std::cerr << library.err_msg() << std::endl;
    }
    exit(library.err_code());
  }
  if(opts.qualified_prefix) {
    compressor->set_name(*opts.qualified_prefix);
  }

  pressio_metrics metrics = library.get_metrics(std::begin(opts.metrics_ids), std::end(opts.metrics_ids));
  if (!metrics) {
    if(rank == 0) {
    std::cerr << "failed to initialize the metrics" << std::endl;
    std::copy(std::begin(opts.metrics_ids),
              std::end(opts.metrics_ids),
              std::ostream_iterator<const char*>(std::cerr, " "));
    std::cerr << library.err_msg() << std::endl;
    }
    exit(library.err_code());
  }

  set_options_from_multimap(*metrics, opts.metrics_options, "metrics");
  compressor->set_metrics(metrics);


  set_early_options(*compressor, opts.early_options);
  set_options_from_multimap(*compressor, opts.options, "compressor");

  return compressor;
}

std::vector<pressio_data> compress(struct pressio_compressor& compressor, cmdline_options const& opts) {

  std::vector<pressio_data> compressed(opts.num_compressed.value_or(opts.input.size()), pressio_data::empty(pressio_byte_dtype, 0, nullptr));
  std::vector<const pressio_data*> inputs_ptrs(opts.input.size());
  std::vector<pressio_data*> compressed_ptrs(compressed.size());
  for (size_t i = 0; i < inputs_ptrs.size(); ++i) {
    inputs_ptrs[i] = &opts.input[i];
  }
  for (size_t i = 0; i < compressed_ptrs.size(); ++i) {
    compressed_ptrs[i] = &compressed[i];
  }


  if (compressor->compress_many(
        compat::data(inputs_ptrs),
        compat::data(inputs_ptrs) + compat::size(inputs_ptrs),
        compat::data(compressed_ptrs),
        compat::data(compressed_ptrs) + compat::size(compressed_ptrs)
        )) {
    if(rank == 0) {
      std::cerr << compressor->error_msg() << std::endl;
    }
    exit(compressor->error_code());
  }
  return compressed;
}

std::vector<pressio_data> decompress(struct pressio_compressor& compressor, std::vector<pressio_data> const& compressed,  cmdline_options const& opts) {
  std::vector<pressio_data> output_buffer;
  std::vector<const pressio_data*> compressed_ptrs(compressed.size());
  std::vector<pressio_data*> output_buffer_ptrs;
  for (size_t i = 0; i < compressed.size(); ++i) {
    compressed_ptrs[i] = &compressed[i];
  }
  for (auto const& input : opts.input) {
    output_buffer.emplace_back(pressio_data::clone(input));
  }
  for (auto& output: output_buffer) {
    output_buffer_ptrs.emplace_back(&output);
  }

  if (compressor->decompress_many(
        compat::data(compressed_ptrs),
        compat::data(compressed_ptrs) + compat::size(compressed_ptrs),
        output_buffer_ptrs.data(),
        compat::data(output_buffer_ptrs) + compat::size(output_buffer_ptrs)
        )) {
    if(rank == 0) {
      std::cerr << compressor->error_msg() << std::endl;
    }
    exit(compressor->error_code());
  }
  return output_buffer;
}

int
main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    auto opts = parse_args(argc, argv);
    pressio library;

    if(opts.actions.find(Action::Version) != opts.actions.end()) {
      print_versions(library);
    }

    if (contains_one_of(opts.actions, Action::Compress, Action::Decompress, Action::Settings)) {

      auto compressor = setup_compressor(library, opts);
      auto options = compressor->get_options();
      auto metrics = compressor->get_metrics();
      std::vector<pressio_data> compressed;
      std::vector<pressio_data> decompressed;

      if (contains(opts.actions, Action::Settings)) {
        print_selected_options(options, std::begin(opts.print_options), std::end(opts.print_options));
        print_selected_options(compressor->get_configuration(), std::begin(opts.print_compile_options), std::end(opts.print_compile_options));
        print_selected_options(metrics->get_options(), std::begin(opts.print_metrics_options), std::end(opts.print_metrics_options));

        for (const auto& input_file_action : opts.input_file_action) {
          print_selected_options(input_file_action->get_options(), std::begin(opts.print_io_input_options), std::end(opts.print_io_input_options));
        }
        for (auto const& compressed_file_action : opts.compressed_file_action) {
          print_selected_options(compressed_file_action->get_options(), std::begin(opts.print_io_comp_options), std::end(opts.print_io_comp_options));
        }
        for (auto const& decompressed_file_action : opts.decompressed_file_action) {
          print_selected_options(decompressed_file_action->get_options(), std::begin(opts.print_io_decomp_options), std::end(opts.print_io_decomp_options));
        }
      }
      
      if (contains(opts.actions, Action::Compress)) {
        compressed = compress(compressor, opts);

        for (size_t i = 0; i < compressed.size(); ++i ) {
          if (auto result = pressio_io_write(&opts.compressed_file_action[i], &compressed[i])) {
            if(rank == 0) {
              std::cerr << "writing compressed file failed " << pressio_io_error_msg(&opts.compressed_file_action[i]) << std::endl;
            }
            exit(EXIT_FAILURE);
          }
        }

      } else if (contains(opts.actions, Action::Decompress)) {
        for (auto const& i: opts.input) {
          compressed.emplace_back(pressio_data::nonowning(i.dtype(), i.data(), i.dimensions()));
        }
      }

      if (contains(opts.actions, Action::Decompress)) {
        distributed::comm::bcast(compressed, 0, MPI_COMM_WORLD);
        decompressed = decompress(compressor, compressed, opts);
      }
      for (size_t i = 0; i < decompressed.size(); ++i) {
        if (auto result = pressio_io_write(&opts.decompressed_file_action[i], &decompressed[i])) {
          if(rank == 0) {
            std::cerr << "writing decompressed file failed " << pressio_io_error_msg(&opts.decompressed_file_action[i]) << std::endl;
          }
          exit(EXIT_FAILURE);
        }
      }

      print_selected_options(compressor->get_metrics_results(), std::begin(opts.print_metrics), std::end(opts.print_metrics));
    }
  }
  MPI_Finalize();
  return 0;
}
