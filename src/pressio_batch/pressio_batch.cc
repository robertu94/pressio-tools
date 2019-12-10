#include <iostream>
#include <iterator>
#include <string>

#include "compressor_configs.h"
#include "datasets.h"
#include <libpressio.h>
#include <libpressio_ext/cpp/printers.h>
#include <unistd.h>

std::ostream&
print_value(std::ostream& out, pressio_option const& opt)
{
  switch (opt.type()) {
    case pressio_option_charptr_type:
      return out << '"' << opt.get_value<std::string>() << '"';
    case pressio_option_float_type:
      return out << opt.get_value<float>();
    case pressio_option_double_type:
      return out << opt.get_value<double>();
    case pressio_option_int32_type:
      return out << opt.get_value<int>();
    case pressio_option_uint32_type:
      return out << opt.get_value<unsigned int>();
    default:
      return out;
  }
}

void
output_csv(std::ostream& out, std::string const& configuration,
           pressio_options* options, std::vector<std::string> const& fields)
{
  out << configuration << ',';
  for (auto const& field : fields) {
    auto const& option = options->get(field);
    print_value(out, option);
    if (field == fields.back())
      out << '\n';
    else
      out << ',';
  }
  std::flush(out);
}

struct cmdline
{
  std::vector<const char*> metrics;
  std::vector<std::string> fields;
  std::string datasets = "./datasets.json";
  std::string compressors = "./compressors.json";
  unsigned int replicats = 1;
};

void
usage()
{
  std::cerr << R"(
pressio_batch [args] [metrics...]
-c compressor_config_file path to the compressor configuration, default: "./compressors.json"
-d dataset_config_file path to the dataset configuration, default: "./datasets.json"
-r replicats the number of times to replicate each configuration, default: 1
-m metrics_ids what metrics_ids to load, default: {time, size, error_stat}
)";
};

cmdline
parse_args(int argc, char* argv[])
{
  cmdline args;

  int opt;
  while ((opt = getopt(argc, argv, "c:d:r:m:")) != -1) {
    switch (opt) {
      case 'd':
        args.datasets = optarg;
        break;
      case 'c':
        args.compressors = optarg;
        break;
      case 'r':
        args.replicats = std::stoi(optarg);
        break;
      case 'm':
        args.metrics.push_back(optarg);
        break;
      default:
        break;
    }
  }
  if (args.metrics.empty()) {
    args.metrics.push_back("time");
    args.metrics.push_back("size");
    args.metrics.push_back("error_stat");
  }
  for (int i = optind; i < argc; ++i) {
    args.fields.push_back(argv[i]);
  }

  return args;
}

int
main(int argc, char* argv[])
{
  auto args = parse_args(argc, argv);

  auto library = pressio_instance();
  auto metrics =
    pressio_new_metrics(library, args.metrics.data(), args.metrics.size());

  auto datasets = load_datasets(args.datasets);
  auto compressor_configs = load_compressors(args.compressors);
  bool first = true;

  for (auto& dataset : datasets) {
    auto input = dataset->load();
    for (auto& compressor_factory : compressor_configs) {
      auto compressor = compressor_factory->load(library);
      pressio_compressor_set_metrics(compressor, metrics);

      for (int i = 0; i < args.replicats; ++i) {

        auto compressed =
          pressio_data_new_empty(pressio_byte_dtype, 0, nullptr);
        auto decompressed = pressio_data_new_clone(input);
        if (pressio_compressor_compress(compressor, input, compressed)) {
          std::cerr << "compression failed" << std::endl;
          pressio_data_free(compressed);
          pressio_data_free(decompressed);
          pressio_compressor_release(compressor);
          continue;
        }
        if (pressio_compressor_decompress(compressor, compressed,
                                          decompressed)) {
          std::cerr << "decompression failed" << std::endl;
          pressio_data_free(compressed);
          pressio_data_free(decompressed);
          pressio_compressor_release(compressor);
          continue;
        }

        auto metrics_results =
          pressio_compressor_get_metrics_results(compressor);
        if (first) {
          first = false;
          if (args.fields.empty())
            std::transform(std::begin(*metrics_results),
                           std::end(*metrics_results),
                           std::back_inserter(args.fields),
                           [](auto const& iterator) { return iterator.first; });
          std::cout << "configuration";
          for (auto const& field : args.fields) {
            std::cout << ',' << field;
          }
          std::cout << std::endl;
        }
        output_csv(std::cout,
                   dataset->get_name() + compressor_factory->get_name(),
                   metrics_results, args.fields);
        pressio_options_free(metrics_results);
        pressio_data_free(compressed);
        pressio_data_free(decompressed);
      }
      pressio_compressor_release(compressor);
    }
    pressio_data_free(input);
  }
  pressio_metrics_free(metrics);
  pressio_release(library);
  return 0;
}
