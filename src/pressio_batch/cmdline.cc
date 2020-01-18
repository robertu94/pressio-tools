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
-m metrics_ids what metrics_ids to load, default: {time, size, error_stat}
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
        args.metrics.push_back(optarg);
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
