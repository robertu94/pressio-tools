#include <vector>
#include <string>

struct cmdline
{
  std::vector<std::string> fields;
  std::string datasets = "./datasets.json";
  std::string compressors = "./compressors.json";
  std::string metrics = "./metrics.json";
  std::string decompressed_dir;
  std::string compressed_dir;
  unsigned int replicats = 1;
  int error_code = 0;
};

cmdline
parse_args(int argc, char* argv[], bool verbose = false);
