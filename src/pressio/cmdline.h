#ifndef PRESSIO_TOOLS_CMDLINE
#define PRESSIO_TOOLS_CMDLINE
#include <set>
#include <map>
#include <vector>
#include <optional>
#include <functional>
#include <pressio_data.h>
#include "file_action.h"

enum class Action
{
  Version,
  Compress,
  Decompress,
  Settings
};

struct options
{
  std::set<Action> actions;
  std::map<std::string, std::string> options;
  std::set<std::string> print_options;
  std::string compressor;
  std::vector<const char*> metrics_ids;
  std::set<const char*> print_metrics;
  std::set<const char*> print_compile_options;
  FileAction compressed_file_action = noop();
  FileAction decompressed_file_action = noop();
  pressio_data* input = nullptr;
};

options parse_args(int argc, char* argv[]);

#endif /*PRESSIO_TOOLS_CMDLINE*/
