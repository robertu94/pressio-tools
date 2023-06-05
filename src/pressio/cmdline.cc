#include "cmdline.h"
#include <iostream>
#include <memory>
#include <std_compat/optional.h>
#include <utility>
#include <vector>
#include <unistd.h>
#include <dlfcn.h>

#include <pressio_version.h>
#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>
#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/printers.h>
#include <libpressio_ext/io/pressio_io.h>

#include "utils/fuzzy_matcher.h"
#include "utils/string_options.h"
#include "utils/pressio_tools_version.h"
#include "options.h"

#if LIBPRESSIO_TOOLS_HAS_MPI
#include <mpi.h>
#endif


namespace {

static int cmdline_rank = 0;

void
usage()
{
  if(cmdline_rank == 0) {
    std::cerr << R"(pressio [args] [compressor]
operations:
-a <action> the actions to preform: compress, decompress, version, settings, help default=compress+decompress
-Q enable fully-qualified mode, this will change the names of options for compressors
-j enable JSON output mode

input datasets:
-d <dim> dimension of the dataset
-t <pressio_type> type of the dataset
-i <input_file> path to the input file
-I <dataset> treat the file as HDF5 and read this dataset
-u <option>=<value> pass the specified option to the generic IO plugin for the input (uncompressed) file
-U <option_key> prints this option after setting it, defaults none, "all" prints all options for the input (uncompressed) file
-T <format> set the input format

-p indicates that all subsequent input dataset arguments are for the "next buffer"

output datasets:

-f <format> compressed file format
-w <compressed_file> use POSIX if format is not set
-s <compressed_file_dataset> use HDF output for the compressed file
-y <option>=<value> pass the specified option to the generic IO plugin for the compressed file
-z <option_key> prints this option after setting it, defaults none, "all" prints all options for the compressed file
-k <num> number of compressed data datasets to pass to compress, defaults to the number of "-p" flags passed + 1

-F <format> decompressed file format
-W <decompressed_file> use POSIX if format is not set
-S <decompressed_file_dataset> use HDF output for the decompressed file
-Y <option>=<value> pass the specified option to the generic IO plugin for the decompressed file
-Z <option_key> prints this option after setting it, defaults none, "all" prints all options for the decompressed file

metrics:
-n <option>=<value> the option key to set to value, default none 
-N <option_key> prints this option after setting it, defaults none, "all" prints all options for the decompressed file
-m <metrics_id> id of a metrics plugin, default none
-M <metrics_id> specific metrics to print, default none, "all" prints all options

options:
-b <key>=<value> set the option early
-o <key>=<value> the option key to set to value, default none
-O <option_key> prints this option after setting it, defaults none, "all" prints all options

configuration:
-C <option_key> prints this configuration key, defaults none, "all" prints all options

compressors: )";

    auto* pressio = pressio_instance();
    std::cerr << pressio_supported_compressors() << std::endl;
    std::cerr << "io: " << pressio_supported_io_modules() << std::endl;
    std::cerr << "metrics: " << pressio_supported_metrics() << std::endl;
    pressio_release(pressio);
  }
}

pressio_dtype
parse_type(std::string const& optarg_s)
{
  std::vector<std::string> types{ "float",  "double", "int8",  "int16",
                                  "int32",  "int64",  "uint8", "uint16",
                                  "uint32", "uint64" };
  auto index = fuzzy_match(optarg_s, std::begin(types), std::end(types));
  if (index) {
    switch (*index) {
      case 0:
        return pressio_float_dtype;
      case 1:
        return pressio_double_dtype;
      case 2: return pressio_int8_dtype; case 3:
        return pressio_int16_dtype;
      case 4:
        return pressio_int32_dtype;
      case 5:
        return pressio_int64_dtype;
      case 6:
        return pressio_uint8_dtype;
      case 7:
        return pressio_uint16_dtype;
      case 8:
        return pressio_uint32_dtype;
      case 9:
        return pressio_uint64_dtype;
      default:
        // intensional fall-through
        (void)0;
    }
  }
  if(cmdline_rank == 0) {
    std::cerr << "invalid type: " << optarg_s << std::endl;
    usage();
  }
  exit(EXIT_FAILURE);
}

std::pair<std::string, std::string>
parse_option (std::string const& option) {
  std::pair<std::string, std::string> result;
  auto sep = option.find("=");
  if (sep != std::string::npos) {
    result.first = option.substr(0, sep);
    result.second = option.substr(sep + 1);
  } else {
    std::string err_msg("invalid argument, no value: ");
    err_msg += option;
    throw std::runtime_error(std::move(err_msg));
  }

  if(result.first.find(":") == std::string::npos) {
    result.first = "pressio:" + result.first;
  }
  
  return result;
}

Action parse_action(std::string const& action) {
  std::vector<std::string> actions { "compress", "decompress", "versions", "settings", "help", "graph", "save", "load"};
  auto id = fuzzy_match(action, std::begin(actions), std::end(actions));
  if(id) {
    switch(*id)
    {
      case 0:
        return Action::Compress;
      case 1:
        return Action::Decompress;
      case 2:
        return Action::Version;
      case 3:
        return Action::Settings;
      case 4:
        return Action::Help;
      case 5:
        return Action::Graph;
      case 6:
        return Action::SaveConfig;
      case 7:
        return Action::LoadConfig;
      default:
        (void)0;
    }
  }

  std::cerr << "invalid action: " << action << std::endl;
  usage();
  exit(EXIT_FAILURE);

}

class io_builder {
  public:
  void set_format(std::string const& format) {
    io_format = format;
  }

  template <class Predicate>
  void set_format_if(std::string const& format, Predicate predicate) {
    if(!io_format || predicate(*io_format)) {
      io_format = format;
    }
  }
  void set_format_if(std::string const& format) {
    set_format_if(format, [](std::string const&){return false;});
  }

  void set_type(pressio_dtype dtype) {
    type = dtype;
  }
  void push_dim(size_t dim) {
    dims.push_back(dim);
  }
  template <class... T>
  void emplace_option(T&&... setting) {
    io_options.emplace(std::forward<T>(setting)...);
  }

  pressio_io make_io() const {
    auto library = pressio();
    auto io_format_str = io_format.value_or("noop");
    pressio_io io = library.get_io(io_format_str);
    if(!io) {
      std::cerr << "failed to get io module " << io_format_str << std::endl;
      exit(EXIT_FAILURE);
    }
    if(io_options.find("io:path") != io_options.end()) {
      if(io_options.count("io:path") == 1) {
        std::string io_path = io_options.find("io:path")->second;
        io->set_options({
            {"io:path", io_path}
        });
      } else {
        std::cerr << "multiple io_paths not supported";
        exit(EXIT_FAILURE);
      }
    }
    compat::optional<pressio_options> null;
    set_options_from_multimap(*io, io_options, "io", null);
    return io;
  }
  std::unique_ptr<pressio_data> make_input_desc() const {
    return (type)
      ? std::make_unique<pressio_data>(pressio_data::owning(*type, dims.size(), dims.data()))
      : std::unique_ptr<pressio_data>(nullptr);
  };

  private:
  compat::optional<pressio_dtype> type;
  std::vector<size_t> dims;
  compat::optional<std::string> io_format;
  std::multimap<std::string, std::string> io_options;
};

}

cmdline_options
parse_args(int argc, char* argv[])
{
#if LIBPRESSIO_TOOLS_HAS_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &cmdline_rank);
#endif
  int opt;
  cmdline_options opts;
#if (LIBPRESSIO_MAJOR_VERSION > 1) || (LIBPRESSIO_MAJOR_VERSION == 0  && LIBPRESSIO_MINOR_VERSION > 75)
  opts.compressor = "pressio";
#else
  opts.compressor = "noop";
#endif
  std::set<Action> actions;

  std::vector<io_builder> input_builder(1);
  std::vector<io_builder> compressed_builder(1);
  std::vector<io_builder> decompressed_builder(1);

  if(argc == 1) {
    usage();
    exit(0);
  }

  while ((opt = getopt(argc, argv, "a:b:d:D:g:t:i:jl:I:u:U:T:f:w:s:y:z:F:W:S:Y:Z:m:M:n:N:o:pO:C:Q")) != -1) {
    switch (opt) {
      case 'a':
        actions.emplace(parse_action(optarg));
        break;
      case 'b':
        opts.early_options.emplace(parse_option(optarg));
        break;
      case 'C':
        opts.print_compile_options.emplace(optarg);
        break;
      case 'd':
        input_builder.back().push_dim(std::stoull(optarg));
        break;
      case 'D':
        opts.extra_dl_handles.push_back(dlopen(optarg, RTLD_LAZY | RTLD_GLOBAL));
        if(opts.extra_dl_handles.back() == nullptr) {
          std::cerr << "failed loading: " << optarg << ": " << dlerror() << std::endl;
          exit(1);
        }
        break;
      case 'f':
        compressed_builder.back().set_format(optarg);
        break;
      case 'F':
        decompressed_builder.back().set_format(optarg);
        break;
      case 'g':
        opts.graph_format = optarg;
        break;
      case 'j':
        opts.format = OutputFormat::JSON;
        break;
      case 'l':
        opts.config_file = optarg;
        break;
      case 'i':
#if LIBPRESSIO_MAJOR_VERSION > 0 || (LIBPRESSIO_MAJOR_VERSION == 0 && LIBPRESSIO_MINOR_VERSION >= 89)
        input_builder.back().set_format_if("by_extension");
#else
        input_builder.back().set_format_if("posix");
#endif
        input_builder.back().emplace_option("io:path", optarg);
        break;
      case 'I':
#if LIBPRESSIO_MAJOR_VERSION > 0 || (LIBPRESSIO_MAJOR_VERSION == 0 && LIBPRESSIO_MINOR_VERSION >= 89)
        input_builder.back().set_format_if("hdf5", [](std::string const& s) {return s == "by_extension";});
#else
        input_builder.back().set_format_if("hdf5", [](std::string const& s) {return s == "posix";});
#endif
        input_builder.back().emplace_option("hdf5:dataset", optarg);
        break;
      case 'k':
        opts.num_compressed = std::stoull(optarg);
        break;
      case 'm':
        opts.metrics_ids.push_back(optarg);
        break;
      case 'M':
        opts.print_metrics.emplace(optarg);
        break;
      case 'n':
        opts.metrics_options.emplace(parse_option(optarg));
        break;
      case 'N':
        opts.print_metrics_options.emplace(optarg);
        break;
      case 'o':
        opts.options.emplace(parse_option(optarg));
        break;
      case 'O':
        opts.print_options.emplace(optarg);
        break;
      case 'p':
        input_builder.emplace_back();
        compressed_builder.emplace_back();
        decompressed_builder.emplace_back();
        break;
      case 'w':
        compressed_builder.back().set_format_if("posix");
        compressed_builder.back().emplace_option("io:path", optarg);
        break;
      case 'W':
        decompressed_builder.back().set_format_if("posix");
        decompressed_builder.back().emplace_option("io:path", optarg);
        break;
      case 'Q':
        opts.qualified_prefix = "pressio";
        break;
      case 's':
        compressed_builder.back().set_format_if("hdf5", [](std::string const& s) {return s == "posix";});
        compressed_builder.back().emplace_option("hdf5:dataset", optarg);
        break;
      case 'S':
        decompressed_builder.back().set_format_if("hdf5", [](std::string const& s) {return s == "posix";});
        decompressed_builder.back().emplace_option("hdf5:dataset", optarg);
        break;
      case 't':
        input_builder.back().set_type(parse_type(optarg));
        break;
      case 'T':
        input_builder.back().set_format(optarg);
        break;
      case 'u':
        input_builder.back().emplace_option(parse_option(optarg));
        break;
      case 'U':
        opts.print_io_input_options.emplace(optarg);
        break;
      case 'y':
        compressed_builder.back().emplace_option(parse_option(optarg));
        break;
      case 'Y':
        decompressed_builder.back().emplace_option(parse_option(optarg));
        break;
      case 'z':
        opts.print_io_comp_options.emplace(optarg);
        break;
      case 'Z':
        opts.print_io_decomp_options.emplace(optarg);
        break;
      default:
        usage();
        exit(EXIT_FAILURE);
    }
  }
  if (actions.empty()) opts.actions = {Action::Compress, Action::Decompress, Action::Settings};
  else opts.actions = std::move(actions);

  if(contains_one_of(opts.actions, {Action::Compress, Action::Settings, Action::Decompress, Action::Help, Action::SaveConfig, Action::LoadConfig})) {
    if(optind < argc) {
      opts.compressor = argv[optind++];
    }
  }


  for (auto const& input_buffer : input_builder) {
    opts.input_file_action.emplace_back(input_buffer.make_io());
    pressio_data* read_data = opts.input_file_action.back()->read(input_buffer.make_input_desc().get());
    if(contains_one_of(opts.actions, {Action::Compress, Action::Decompress})) {
      if(read_data == nullptr) {
        if(cmdline_rank == 0) {
          std::cerr << "failed to read input file " << pressio_io_error_msg(&opts.input_file_action.back()) << std::endl;
        }
        exit(EXIT_FAILURE);
      } else {
        opts.input.emplace_back(std::move(*read_data));
        delete read_data;
      }
    }
  }


  for(size_t i = 0; i < compressed_builder.size(); ++i) {
    opts.compressed_file_action.emplace_back(compressed_builder[i].make_io());
  }
  for (size_t i = 0; i < decompressed_builder.size(); ++i) {
    opts.decompressed_file_action.emplace_back(decompressed_builder[i].make_io());
  }
  return opts;
}
