#ifndef PRESSIO_TOOLS_CMDLINE
#define PRESSIO_TOOLS_CMDLINE
#include <set>
#include <map>
#include <vector>
#include <optional>
#include <functional>
#include <pressio_data.h>
#include <libpressio_ext/io/pressio_io.h>


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
  std::set<const char*> print_metrics_options;
  std::set<const char*> print_io_input_options;
  std::set<const char*> print_io_decomp_options;
  std::set<const char*> print_io_comp_options;
  pressio_data* input = nullptr;
  pressio_io* input_file_action;
  pressio_io* compressed_file_action;
  pressio_io* decompressed_file_action;
};

options parse_args(int argc, char* argv[]);

#endif /*PRESSIO_TOOLS_CMDLINE*/
