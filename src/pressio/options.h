#include <map>
#include <string>
#include <libpressio_ext/cpp/options.h>
#include <std_compat/optional.h>
struct pressio_configurable;
int set_options_from_multimap(pressio_configurable& c, std::multimap<std::string,std::string> const& user_options, const char* configurable_type, compat::optional<pressio_options>& out);
extern int rank;
