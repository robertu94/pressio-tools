#include "cmdline.h"
#include <iostream>
#include <vector>
#include <unistd.h>

#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>
#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/io.h>
#include <libpressio_ext/io/pressio_io.h>


#include "utils/fuzzy_matcher.h"


namespace {
void
usage()
{
  std::cerr << R"(pressio [args] [compressor]
operations:
-a <action> the actions to preform: compress, decompress, version, settings, default=compress+decompress

input datasets:
-d <dim> dimension of the dataset
-t <pressio_type> type of the dataset
-i <input_file> path to the input file
-I <dataset> treat the file as HDF5 and read this dataset
-u <option>=<value> pass the specified option to the generic IO plugin for the input (uncompressed) file
-U <option_key> prints this option after setting it, defaults none, "all" prints all options for the input (uncompressed) file
-T <format> set the input format

output datasets:

-f <format> compressed file format
-w <compressed_file> use POSIX if format is not set
-s <compressed_file_dataset> use HDF output for the compressed file
-y <option>=<value> pass the specified option to the generic IO plugin for the compressed file
-z <option_key> prints this option after setting it, defaults none, "all" prints all options for the compressed file

-F <format> decompressed file format
-W <decompressed_file> use POSIX if format is not set
-S <decompressed_file_dataset> use HDF output for the decompressed file
-Y <option>=<value> pass the specified option to the generic IO plugin for the decompressed file
-Z <option_key> prints this option after setting it, defaults none, "all" prints all options for the decompressed file

metrics:
-m <metrics_id> id of a metrics plugin, default none
-M <metrics_id> specific metrics to print, default none, "all" prints all options

options:
-o <key>=<value> the option key to set to value, default none
-O <option_key> prints this option after setting it, defaults none, "all" prints all options

configuration:
-C <option_key> prints this configuration key, defaults none, "all" prints all options

compressors: )";

  auto* pressio = pressio_instance();
  std::cerr << pressio_supported_compressors() << std::endl;
  std::cerr << "io: " << pressio_supported_io_modules() << std::endl;

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
  std::cerr << "invalid type: " << optarg_s << std::endl;
  usage();
  exit(EXIT_FAILURE);
}

std::pair<std::string, std::string>
parse_option (std::string const& option) {
  std::pair<std::string, std::string> result;
  if (auto sep = option.find("="); sep != std::string::npos) {
    result.first = option.substr(0, sep);
    result.second = option.substr(sep + 1);
  } else {
    std::string err_msg("invalid argument, no value: ");
    err_msg += option;
    throw std::runtime_error(std::move(err_msg));
  }
  
  return result;
}

Action parse_action(std::string const& action) {
  std::vector<std::string> actions { "compress", "decompress", "versions", "settings" };
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
      default:
        (void)0;
    }
  }

  std::cerr << "invalid action: " << action << std::endl;
  usage();
  exit(EXIT_FAILURE);

}

pressio_io* make_io(std::string const& format, std::map<std::string, std::string> const& format_options)
{
  auto library = pressio_instance();
  auto io = pressio_get_io(library, format.c_str());
  if(io == nullptr) {
    std::cerr << "failed to get io module " << format << std::endl;
    exit(EXIT_FAILURE);
  }
  auto options = pressio_io_get_options(io);
  for (auto const& key_value : format_options) {
    auto value = pressio_option_new_string(key_value.second.c_str());
    if(pressio_options_cast_set(options, key_value.first.c_str(), value, pressio_conversion_special) != pressio_options_key_set) {
      std::cerr << "failed to set" << key_value.first << " to " << key_value.second << std::endl;
      exit(EXIT_FAILURE);
    }
    pressio_option_free(value);
  }
  pressio_io_set_options(io, options);

  pressio_release(library);
  return io;
}
}

options
parse_args(int argc, char* argv[])
{
  int opt;
  options opts;
  std::vector<size_t> dims;
  std::optional<pressio_dtype> type;
  std::set<Action> actions;

  std::optional<std::string> input_io_format;
  std::optional<std::string> compressed_io_format;
  std::optional<std::string> decompressed_io_format;
  std::map<std::string, std::string> input_io_options;
  std::map<std::string, std::string> decompressed_io_options;
  std::map<std::string, std::string> compressed_io_options;

  while ((opt = getopt(argc, argv, "a:d:t:i:I:u:U:T:f:w:s:y:z:F:W:S:Y:Z:m:M:o:O:C:")) != -1) {
    switch (opt) {
      case 'a':
        actions.emplace(parse_action(optarg));
        break;
      case 'C':
        opts.print_compile_options.emplace(optarg);
        break;
      case 'd':
        dims.push_back(std::stoull(optarg));
        break;
      case 'f':
        compressed_io_format = optarg;
        break;
      case 'F':
        decompressed_io_format = optarg;
        break;
      case 'i':
        if(!input_io_format) input_io_format = "posix";
        input_io_options["io:path"] = optarg;
        break;
      case 'I':
        if(input_io_format || input_io_format=="posix") input_io_format = "hdf5";
        input_io_options["hdf5:dataset"] = optarg;
        break;
      case 'm':
        opts.metrics_ids.push_back(optarg);
        break;
      case 'M':
        opts.print_metrics.emplace(optarg);
        break;
      case 'o':
        opts.options.emplace(parse_option(optarg));
        break;
      case 'O':
        opts.print_options.emplace(optarg);
        break;
      case 'w':
        if(!compressed_io_format) compressed_io_format = "posix";
        compressed_io_options["io:path"] = optarg;
        break;
      case 'W':
        if(!decompressed_io_format) decompressed_io_format = "posix";
        decompressed_io_options["io:path"] = optarg;
        break;
      case 's':
        if(compressed_io_format || compressed_io_format=="posix") compressed_io_format = "hdf5";
        compressed_io_options["hdf5:dataset"] = optarg;
        break;
      case 'S':
        if(decompressed_io_format || decompressed_io_format=="posix") decompressed_io_format = "hdf5";
        decompressed_io_options["hdf5:dataset"] = optarg;
        break;
      case 't':
        type = parse_type(optarg);
        break;
      case 'T':
        input_io_format = optarg;
        break;
      case 'u':
        input_io_options.emplace(parse_option(optarg));
        break;
      case 'U':
        opts.print_io_input_options.emplace(optarg);
        break;
      case 'y':
        compressed_io_options.emplace(parse_option(optarg));
        break;
      case 'Y':
        decompressed_io_options.emplace(parse_option(optarg));
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
  if (actions.empty()) opts.actions = {Action::Compress, Action::Decompress};
  else opts.actions = std::move(actions);

  if(opts.actions.find(Action::Compress) != actions.end() or 
      opts.actions.find(Action::Settings) != actions.end() or 
      opts.actions.find(Action::Decompress) != actions.end()
      ) {
    if(optind >= argc) {
      std::cerr << "expected positional arguments" << std::endl;
      usage();
      exit(EXIT_FAILURE);
    } else {
      opts.compressor = argv[optind++];
    }
  }


  pressio_data* input_desc = (type && !dims.empty())? pressio_data_new_owning(*type, dims.size(), dims.data()): nullptr;
  opts.input_file_action = make_io(input_io_format.value_or("noop"), input_io_options);
  opts.input = pressio_io_read(opts.input_file_action, input_desc);
  if(opts.input == nullptr and (
    opts.actions.find(Action::Compress) != opts.actions.end() or
    opts.actions.find(Action::Decompress) != opts.actions.end()
    )) {
    std::cerr << "failed to read input file " << pressio_io_error_msg(opts.input_file_action) << std::endl;
    usage();
    exit(EXIT_FAILURE);
  }

  opts.compressed_file_action = make_io(compressed_io_format.value_or("noop"), compressed_io_options);
  opts.decompressed_file_action = make_io(decompressed_io_format.value_or("noop"), decompressed_io_options);
  return opts;
}
