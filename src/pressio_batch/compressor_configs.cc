#include "compressor_configs.h"
#include <iostream>
#include <libpressio.h>
#include <libpressio_ext/cpp/options.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <utils/string_options.h>

namespace pt = boost::property_tree;
using namespace std::literals;

struct compressor_config_impl: public compressor_config {
  std::string name;
  std::string compressor_id;
  std::multimap<std::string, std::string> config_options;
  std::multimap<std::string, std::string> early_config_options;

  pressio_compressor* load(pressio* library) {
    pressio_compressor* compressor = pressio_get_compressor(library, compressor_id.c_str());

    auto early_config_opts = options_from_multimap(early_config_options);
    pressio_options* early_options = pressio_compressor_get_options(compressor);
    if(pressio_compressor_set_options(compressor, early_options)) {
      throw std::runtime_error("failed to set "s + pressio_compressor_error_msg(compressor));
    }

    pressio_options* options = pressio_compressor_get_options(compressor);
    auto config_opts = options_from_multimap(config_options);
    for (auto& [setting, value] : *config_opts) {
      auto status = pressio_options_cast_set(options, setting.c_str(), &value, pressio_conversion_special);
      if(status != pressio_options_key_set) throw std::runtime_error("failed to assign "s + setting);
    }
    if(pressio_compressor_check_options(compressor, options)) {
      throw std::runtime_error("failed to check "s + pressio_compressor_error_msg(compressor));
    }
    if(pressio_compressor_set_options(compressor, options)) {
      throw std::runtime_error("failed to set "s + pressio_compressor_error_msg(compressor));
    }
    pressio_options_free(config_opts);
    pressio_options_free(options);
    pressio_options_free(early_options);
    return compressor;
  }
  std::string const& get_name() { return name; }
};

std::vector<std::unique_ptr<compressor_config>> load_compressors(std::string const& compressor_config_path, bool verbose) {
  std::vector<std::unique_ptr<compressor_config>> compressors;
  pt::ptree compressor_tree;
  pt::read_json(compressor_config_path, compressor_tree);
  for (auto& [path, config] : compressor_tree) {
      if(verbose) std::clog << "loading configuration " << path << std::endl;
      auto compressor_config = std::make_unique<compressor_config_impl>();
      compressor_config->name = path;
      compressor_config->compressor_id = config.get<std::string>("compressor_id");
      for(auto& option: config.get_child("options")) {
        compressor_config->config_options.emplace(option.first, option.second.get_value<std::string>());
      }
      compressors.emplace_back(std::move(compressor_config));
  }
  return compressors;
}
