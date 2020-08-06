#include <vector>
#include <string>
#include <cassert>
#include <utils/string_options.h>
#include <libpressio_ext/cpp/options.h>

pressio_options options_from_multimap(std::multimap<std::string,std::string> const& map) {
  pressio_options opt;
  std::vector<std::string> values;
  std::optional<std::string> key;
  for (auto const& entry : map) {
    if(key && key != entry.first) {
      //we have a new key, create the entry for it
      if (values.size() != 1) {
        opt.set(*key, values);
      } else {
        opt.set(*key, values.front());
      }
      //and update value to the newest value
      values = {entry.second};
    } else {
      //just append the last value saw
      values.emplace_back(entry.second);
    }
    key = entry.first;
  }
  //handle the last keys if there were any
  if(values.size() == 1){
    opt.set(*key, values.front());
  } else if(values.size() > 1) {
    opt.set(*key, values);
  }
  return opt;
}
