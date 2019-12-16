#include <iosfwd>
#include <string>
#include <vector>
struct pressio_options;

struct pressio_option;

std::ostream&
print_value(std::ostream& out, pressio_option const& opt);
void

output_csv(std::ostream& out, std::string const& configuration,
           pressio_options const* options, std::vector<std::string> const& fields);
