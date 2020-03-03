#include <iostream>
#include <iterator>
#include <string>
#include <algorithm>

#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>

#include "compressor_configs.h"
#include "datasets.h"
#include "cmdline.h"
#include "io.h"



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
                   dataset->get_name() + "," + compressor_factory->get_name(),
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
