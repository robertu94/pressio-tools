#include <map>
#include <string>
struct pressio_configurable;
int set_options_from_multimap(pressio_configurable& c, std::multimap<std::string,std::string> const& user_options, const char* configurable_type);
extern int rank;
