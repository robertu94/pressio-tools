#include <map>
#include <pressio_options.h>
#include <pressio_option.h>

struct pressio_options* options_from_multimap(std::multimap<std::string,std::string> const& map);

