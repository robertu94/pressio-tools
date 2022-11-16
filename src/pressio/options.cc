#include <iostream>
#include <libpressio_ext/cpp/libpressio.h>
#include <utils/string_options.h>
#include <utility>
#include <map>
#include "options.h"

int set_options_from_multimap(pressio_configurable& c, std::multimap<std::string,std::string> const& user_options, const char* configurable_type) {
  pressio_options configurable_options = c.get_options();
  pressio_options new_options;
  auto presssio_user_opts = options_from_multimap(user_options);
  for (auto it =  presssio_user_opts.begin(); it != presssio_user_opts.end(); ++it) {
    auto const& setting = it->first;
    auto const& value = it->second;
    auto option_status = configurable_options.key_status(setting);
    if(option_status == pressio_options_key_does_not_exist) {
          if(rank == 0) {
            std::cerr << "non existent option for the " << configurable_type << " : " << setting  << std::endl;
          }
          exit(EXIT_FAILURE);
    }
    auto option = configurable_options.get(setting);
    new_options.set(setting, std::move(option));
    auto status = new_options.cast_set(setting, value, pressio_conversion_special);
    switch(status) {
      case pressio_options_key_does_not_exist:
        {
          if(rank == 0) {
            std::cerr << "non existent option for the " << configurable_type << " : " << setting  << std::endl;
          }
          exit(EXIT_FAILURE);
        }
      case pressio_options_key_exists:
        {
          if(rank == 0) {
            auto type = new_options.get(setting).type();
            std::cerr << "cannot convert to value " << value << " to correct type for setting " << setting  <<
              " which has type " << type 
              << std::endl;
          }
          exit(EXIT_FAILURE);
        }
      default:
        (void)0;
    }
  }
  return c.set_options(new_options);
}
