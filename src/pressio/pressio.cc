#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <fstream>
#include <set>

#include <libpressio.h>
#include <pressio_version.h>
#include <libpressio_ext/cpp/compressor.h>
#include <libpressio_ext/cpp/configurable.h>
#include <libpressio_ext/cpp/io.h>
#include <libpressio_ext/cpp/metrics.h>
#include <libpressio_ext/cpp/options.h>
#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/printers.h>
#include <libpressio_ext/io/pressio_io.h>
#include <libpressio_meta.h>

#include <utils/pressio_tools_version.h>
#include <utils/string_options.h>
#include "options.h"

#if LIBPRESSIO_TOOLS_HAS_MPI
#include <mpi.h>
#include <libdistributed_comm.h>
#include <libpressio_ext/cpp/serializable.h>
#endif

#if LIBPRESSIO_HAS_JSON
#include <libpressio_ext/json/pressio_options_json.h>
#endif

#include "cmdline.h"
#include "graph.h"

int rank = 0;

void print_help(pressio_compressor & compressor, Action action) {
    const bool is_full = (action == Action::FullHelp);
    auto docs = compressor->get_documentation();
    if(docs.key_status("pressio:description") == pressio_options_key_set) {
      std::cout << docs.get("pressio:description").get_value<std::string>() << std::endl;
      docs.erase("pressio:description");
    }
    auto configs = compressor->get_configuration();
    auto options = compressor->get_options();
    auto metrics = compressor->get_metrics();
    auto metrics_results = metrics->get_metrics_results({});
    auto metrics_docs = metrics->get_documentation();
    std::vector<std::string> highlevel;
    configs.get("pressio:highlevel", &highlevel);
    auto  key_is_highlevel = [&](std::string const& key){
        return std::find(std::begin(highlevel), std::end(highlevel), key) != std::end(highlevel);
    };
    auto is_detail = [&](std::string const& key) {
        return (key.find("pressio:") != std::string::npos
                || key.find("predictors:") != std::string::npos
                || key.find("composite:") != std::string::npos
                || key.find("metrics:")!= std::string::npos
                || key.find(compressor->prefix() + std::string(":metric")) != std::string::npos
                ) && !key_is_highlevel(key);
    };

    std::cout <<  std::endl;
    std::cout << "Options" << std::endl;
    std::set<std::string> skip_list;
    for (auto const& i : options) {
      std::string const& key = i.first;
      pressio_option const& value = i.second;
      if(!is_full && is_detail(key)) {
          continue;
      }
      pressio_option_type type = value.type();

      std::cout << key << " <" << type << "> ";
      if(docs.key_status(key) == pressio_options_key_set) {
        std::cout << docs.get(key).get_value<std::string>();
      }
      if(configs.key_status(key) == pressio_options_key_set) {
        std::cout << " {" << configs.get(key) << "}";
        skip_list.emplace(key);
      }
      std::cout << std::endl;
    }

    bool any_configs = false;

    for (auto const& i : configs) {
      std::string const& key = i.first;
      pressio_option const& value = i.second;
      if(!is_full && is_detail(key)) {
          continue;
      }
      if(skip_list.find(key) != skip_list.end())  {
        continue;
      }
      if(! any_configs ) {
        any_configs = true;
        std::cout << std::endl << "Configuration" << std::endl;
      }
      pressio_option_type type = value.type();

      std::cout << key << " <" << type << "> ";
      if(docs.key_status(key) == pressio_options_key_set) {
        std::cout << docs.get(key).get_value<std::string>();
      }
      std::cout << std::endl;
    }

    if(is_full) {
        bool any_others = false;
        for (auto const& i : skip_list) {
          if(!any_others) {
            std::cout << std::endl <<  "Other Configuration" << std::endl;
            any_others = true;
          }
          std::cout << i << std::endl;
        }
    }

    bool any_metrics = false;
    for (auto const& i : metrics_results) {
      if(!any_metrics) {
        std::cout << std::endl <<  "Metrics Results" << std::endl;
        any_metrics = true;
      }
      std::string const& key = i.first;
      pressio_option const& value = i.second;
      auto const& type = value.type();
      std::cout << key << " <" << type << "> ";
      if(metrics_docs.key_status(key) == pressio_options_key_set) {
        std::cout << metrics_docs.get(key).get_value<std::string>();
      }
      std::cout << std::endl;
    }


}

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
void print_selected_options(pressio_options const& options, ForwardIt begin, ForwardIt end, OutputFormat format = OutputFormat::Human) {
  if(rank == 0) {
      std::vector<std::string> keys;
      std::set<std::string> matched_keys;
      std::transform(options.begin(), options.end(),
              std::back_inserter(keys),
              [](auto i) { return i.first; });
      if(std::find(begin, end, std::string("all")) != end) {
        std::copy(keys.begin(), keys.end(), std::inserter(matched_keys, matched_keys.begin()));
      } else {
        std::for_each(begin, end, [&](std::string const& pattern) {
            std::regex r{pattern};
            std::copy_if(keys.begin(), keys.end(), std::inserter(matched_keys, matched_keys.begin()), [&](std::string const& key) {
                        return std::regex_search(key, r);
                    });
        });
      }
  switch(format) {
    case OutputFormat::Human:
      std::for_each(
          matched_keys.begin(),
          matched_keys.end(),
          [options](std::string const key){
              try {
                std::cerr << key << options.get(key) << std::endl;
              } catch(std::out_of_range const&) {
                std::cerr << ": option is unknown" << std::endl;
                exit(EXIT_FAILURE);
              }
          });
        break;
      case OutputFormat::JSON:
#if LIBPRESSIO_HAS_JSON
        pressio_options for_json;
        if(!matched_keys.empty()) {
            for(std::string const& key : matched_keys) {
              pressio_option const& value = options.get(key);
              for_json.set(key, value);
            }
          char* json = pressio_options_to_json(nullptr, &for_json);
          std::cout << json << std::endl;
          free(json);
        }
#else
        std::cerr << "JSON support not included in libpressio" << std::endl;
        exit(1);
#endif
        break;
      }
  }
}


template <class PressioObject>
void set_early_options(PressioObject& c, std::multimap<std::string,std::string> const& user_options) {
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
  if(contains(opts.actions, Action::LoadConfig)) {
#if LIBPRESSIO_HAS_JSON
      std::ostringstream oss;
      std::ifstream in (opts.config_file);
      if(!in) {
          std::cerr << "failed to read file " << opts.config_file << std::endl;
          exit(1);
      }
      oss << in.rdbuf();
      std::string json_str(oss.str());
      pressio_options* opts = pressio_options_new_json(&library, json_str.c_str());
      if(!opts) {
          std::cerr << library.err_msg() << std::endl;
          exit(library.err_code());
      }
      if(compressor->set_options(*opts)) {
          std::cerr << compressor->error_msg() << std::endl;
          exit(compressor->error_code());
      }
      pressio_options_free(opts);
#else
      std::cerr << "libpressio built without JSON support" << std::endl;
      exit(1);
#endif
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

  compat::optional<pressio_options> null;
  set_options_from_multimap(*metrics, opts.metrics_options, "metrics", null);
  compressor->set_metrics(metrics);


  compat::optional<pressio_options> out;
  set_early_options(*compressor, opts.early_options);
  if(contains(opts.actions, Action::SaveConfig)) {
      out = options_from_multimap(opts.early_options);
  }
  int rc = set_options_from_multimap(*compressor, opts.options, "compressor", out);
  if(contains(opts.actions, Action::SaveConfig)) {
#if LIBPRESSIO_HAS_JSON
          std::ofstream of(opts.config_file);
          char* str = pressio_options_to_json(&library, &*out);
          of << str;
          free(str);
#else
          std::cerr << "JSON support not included" << std::endl;
          exit(1);
#endif
  }
  if(rc) {
    if(rank == 0) {
      std::cerr << compressor->error_msg() << std::endl;
    }
    exit(compressor->error_code());
  }

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
  libpressio_register_all();
  #if LIBPRESSIO_TOOLS_HAS_MPI
  int requested=MPI_THREAD_MULTIPLE, provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  #endif
  {
    #if LIBPRESSIO_TOOLS_HAS_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    #else
    rank = 0;
    #endif
    auto opts = parse_args(argc, argv);
    pressio library;

    if(opts.actions.find(Action::Version) != opts.actions.end()) {
      print_versions(library);
    }

    if (contains_one_of(opts.actions, {Action::Compress, Action::Decompress, Action::Settings, Action::Help, Action::Graph, Action::SaveConfig, Action::LoadConfig, Action::FullHelp})) {

      auto compressor = setup_compressor(library, opts);
      auto options = compressor->get_options();
      auto metrics = compressor->get_metrics();
      std::vector<pressio_data> compressed;
      std::vector<pressio_data> decompressed;

      if (contains(opts.actions, Action::FullHelp)) {
        print_help(compressor, Action::FullHelp);
      } else if (contains(opts.actions, Action::Help)) {
        print_help(compressor, Action::Help);
      }

      if (contains(opts.actions, Action::Settings)) {
        print_selected_options(options, std::begin(opts.print_options), std::end(opts.print_options), opts.format);
        print_selected_options(compressor->get_configuration(), std::begin(opts.print_compile_options), std::end(opts.print_compile_options), opts.format);
        print_selected_options(metrics->get_options(), std::begin(opts.print_metrics_options), std::end(opts.print_metrics_options), opts.format);

        for (const auto& input_file_action : opts.input_file_action) {
          print_selected_options(input_file_action->get_options(), std::begin(opts.print_io_input_options), std::end(opts.print_io_input_options), opts.format);
        }
        for (auto const& compressed_file_action : opts.compressed_file_action) {
          print_selected_options(compressed_file_action->get_options(), std::begin(opts.print_io_comp_options), std::end(opts.print_io_comp_options), opts.format);
        }
        for (auto const& decompressed_file_action : opts.decompressed_file_action) {
          print_selected_options(decompressed_file_action->get_options(), std::begin(opts.print_io_decomp_options), std::end(opts.print_io_decomp_options), opts.format);
        }
      }

      if (contains(opts.actions, Action::Graph)) {
          print_graph(compressor, opts.graph_format);
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
#if LIBPRESSIO_TOOLS_HAS_MPI
        distributed::comm::bcast(compressed, 0, MPI_COMM_WORLD);
#endif
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

      print_selected_options(compressor->get_metrics_results(), std::begin(opts.print_metrics), std::end(opts.print_metrics), opts.format);
    }
  }
  #if LIBPRESSIO_TOOLS_HAS_MPI
  MPI_Finalize();
  #endif
  return 0;
}
