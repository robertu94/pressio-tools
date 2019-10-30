#include "cmdline.h"
#include <iostream>
#include <vector>
#include <unistd.h>

#include <libpressio.h>
#include <libpressio_ext/io/posix.h>
#include <libpressio_ext/io/hdf5.h>


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

output datasets:
-w <compressed_file> 
-W <decompressed_file>
-s <compressed_file_dataset> use HDF output for the compressed file
-S <decompressed_file_dataset> use HDF output for the decompressed file

metrics:
-m <metrics_id> id of a metrics plugin, default none
-M <metrics_id> specific metrics to print, default none, "all" prints all options

options:
-o <key>=<value> the option key to set to value, default none
-O <option_key> prints this option after setting it, defaults none, "all" prints all options

compressors: )";

  auto* pressio = pressio_instance();
  std::cerr << pressio_supported_compressors() << std::endl;

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

pressio_data*
read_source(std::string const& filename, std::vector<size_t> const& dims,
            std::optional<pressio_dtype> const& type, std::optional<std::string> const& dataset)
{
  pressio_data* data = nullptr;
  if(dataset) {
    data = pressio_io_data_path_h5read(filename.c_str(), dataset->c_str());
  } else if (!filename.empty() && type){
    data = pressio_data_new_owning(*type, dims.size(), dims.data());
    if(data == nullptr) return nullptr;
    data = pressio_io_data_path_read(data, filename.c_str());
  } else if (!filename.empty()) {
    data = pressio_io_data_path_read(nullptr, filename.c_str());
  } 

  return data;
}
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

options
parse_args(int argc, char* argv[])
{
  int opt;
  options opts;
  std::string filename;
  std::optional<std::string> dataset;
  std::vector<size_t> dims;
  std::optional<pressio_dtype> type;
  std::set<Action> actions;
  std::optional<std::string> compressed_file, decompressed_file;
  std::optional<std::string> compressed_dataset, decompressed_dataset;

  while ((opt = getopt(argc, argv, "a:C:d:i:I:m:M:o:O:t:w:W:s:S:")) != -1) {
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
      case 'i':
        filename = optarg;
        break;
      case 'I':
        dataset = optarg;
        break;
      case 't':
        type = parse_type(optarg);
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
        compressed_file = optarg;
        break;
      case 'W':
        decompressed_file = optarg;
        break;
      case 's':
        compressed_dataset = optarg;
        break;
      case 'S':
        decompressed_dataset = optarg;
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

  opts.input = read_source(filename, dims, type, dataset);
  if(opts.input == nullptr and (
    actions.find(Action::Compress) != actions.end() or
    actions.find(Action::Decompress) != actions.end()
    )) {
    std::cerr << "failed to read input file" << std::endl;
    usage();
    exit(EXIT_FAILURE);
  }

  if (compressed_file)
  {
    if(compressed_dataset) { opts.compressed_file_action = to_hdf(*compressed_file, *compressed_dataset);}
    else {opts.compressed_file_action = to_binary(*compressed_file);}
  }

  if (decompressed_file)
  {
    if(decompressed_dataset) { opts.decompressed_file_action = to_hdf(*decompressed_file, *decompressed_dataset);}
    else {opts.decompressed_file_action = to_binary(*decompressed_file);}
  }

  return opts;
}
