#include "metrics.h"
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <pressio.h>
#include <pressio_metrics.h>
#include <pressio_option.h>
#include <pressio_options.h>
#include <libpressio_ext/cpp/options.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <utils/string_options.h>

namespace pt = boost::property_tree;
using namespace std::literals;

struct metrics_config_impl : public metrics_config {
  std::vector<std::string> metric_ids;
  std::multimap<std::string,std::string> metrics_options;
  std::multimap<std::string,std::string> early_metrics_options;

  pressio_metrics* load(pressio* library) {
    std::vector<const char*> metrics_id_c;
    std::transform(std::begin(metric_ids),
        std::end(metric_ids),
        std::back_inserter(metrics_id_c),
        [](auto const& metric_id){return metric_id.c_str();});

    pressio_metrics* metric = pressio_new_metrics(library, metrics_id_c.data(), metrics_id_c.size());
    pressio_options* options = pressio_metrics_get_options(metric);

    auto early_opts = options_from_multimap(early_metrics_options);
    pressio_metrics_set_options(metric, early_opts);

    auto metric_opts = options_from_multimap(metrics_options);
    for (auto const& option_pair : *metric_opts) {
      if(pressio_options_cast_set(options, option_pair.first.c_str(), &option_pair.second, pressio_conversion_special)) {
        pressio_options_free(options);
        pressio_metrics_free(metric);
        throw std::runtime_error("invalid option "s + option_pair.first);
      }
    }
    pressio_metrics_set_options(metric, options);
    pressio_options_free(metric_opts);

    return metric;
  }
};

std::unique_ptr<metrics_config> load_metrics(std::string const& metrics_config_path, bool verbose) {
  auto config = std::make_unique<metrics_config_impl>();
  pt::ptree metrics_tree;
  pt::read_json(metrics_config_path, metrics_tree);

  for (auto const& module: metrics_tree.get_child("modules")) {
    config->metric_ids.emplace_back(module.second.get_value<std::string>());
  }
  for (auto const& module: metrics_tree.get_child("options")) {
    config->metrics_options.emplace(
        module.first,
        module.second.get_value<std::string>());
  }
  if(metrics_tree.find("early_options") != metrics_tree.not_found()) {
    for (auto const& module: metrics_tree.get_child("early_options")) {
      config->early_metrics_options.emplace(
          module.first,
          module.second.get_value<std::string>());
    }
  }
  return config;
}
