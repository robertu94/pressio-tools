#ifndef PRESSIO_TOOLS_CMDLINE
#define PRESSIO_TOOLS_CMDLINE
#include <set>
#include <map>
#include <vector>
#include <std_compat/optional.h>
#include <memory>
#include <functional>
#include <algorithm>
#include <libpressio/pressio_version.h>
#include <pressio_data.h>
#include <libpressio_ext/io/pressio_io.h>
#include <libpressio_ext/cpp/data.h>
#include <libpressio_ext/cpp/io.h>

enum class OutputFormat {
  Human,
  JSON
};
enum class Action
{
  Version,
  Compress,
  Decompress,
  SaveConfig,
  LoadConfig,
  Settings,
  Help,
  Graph
};

template <class Set, class Item>
bool contains(Set const& set, Item const& item) {
  return set.find(item) != set.end();
}

template <class Set>
bool contains_one_of(Set const& set, std::initializer_list<Action> items) {
  return std::any_of(std::begin(items), std::end(items), [&set](Action a) {
      return contains(set, a);
      });
  
}


struct cmdline_options
{
  std::set<Action> actions;
  std::multimap<std::string, std::string> early_options;
  std::multimap<std::string, std::string> options;
  std::multimap<std::string, std::string> metrics_options;
  std::set<std::string> print_options;
  std::string compressor;
  std::string config_file;
  std::vector<const char*> metrics_ids;
  std::set<const char*> print_metrics;
  std::set<const char*> print_compile_options;
  std::set<const char*> print_metrics_options;
  std::set<const char*> print_io_input_options;
  std::set<const char*> print_io_decomp_options;
  std::set<const char*> print_io_comp_options;
  std::vector<pressio_data> input;
  std::vector<pressio_io> input_file_action;
  std::vector<pressio_io> compressed_file_action;
  std::vector<pressio_io> decompressed_file_action;
  compat::optional<std::string> qualified_prefix;
  compat::optional<size_t> num_compressed;
  OutputFormat format = OutputFormat::Human;
  std::vector<void*> extra_dl_handles;
  std::string graph_format = "graphviz";
};

cmdline_options parse_args(int argc, char* argv[]);

#endif /*PRESSIO_TOOLS_CMDLINE*/
