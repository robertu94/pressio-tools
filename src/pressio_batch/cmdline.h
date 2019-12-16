#include <vector>
#include <string>

struct cmdline
{
  std::vector<const char*> metrics;
  std::vector<std::string> fields;
  std::string datasets = "./datasets.json";
  std::string compressors = "./compressors.json";
  unsigned int replicats = 1;
};

cmdline
parse_args(int argc, char* argv[]);
