#include "cmdline.h"
#include <iostream>
#include <unistd.h>


static void
usage()
{
  std::cerr << R"(
pressio_batch [args] [metrics...]
-c compressor_config_file path to the compressor configuration, default: "./compressors.json"
-d dataset_config_file path to the dataset configuration, default: "./datasets.json"
-r replicats the number of times to replicate each configuration, default: 1
-m metrics_config file path the metrics configuration, default: "./metrics.json"
-w compressed_dir output the compressed data files to this directory
-W decompressed_dir output the decompressed data files to this directory
)";
};

cmdline
parse_args(int argc, char* argv[], bool verbose)
{
  cmdline args;

  int opt;
  while ((opt = getopt(argc, argv, "c:d:hr:m:w:W:")) != -1) {
    switch (opt) {
      case 'd':
        args.datasets = optarg;
        break;
      case 'c':
        args.compressors = optarg;
        break;
      case 'h':
        if(verbose) usage();
        args.error_code = 1;
        break;
      case 'r':
        args.replicats = std::stoi(optarg);
        break;
      case 'm':
        args.metrics = optarg;
        break;
      case 'w':
        args.compressed_dir = optarg;
        break;
      case 'W':
        args.decompressed_dir = optarg;
        break;
      default:
        break;
    }
  }
  for (int i = optind; i < argc; ++i) {
    args.fields.push_back(argv[i]);
  }

  return args;
}
