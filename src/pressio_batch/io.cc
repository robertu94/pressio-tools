#include "io.h"
#include <libpressio_ext/cpp/printers.h>
#include <libpressio_ext/cpp/options.h>

std::ostream&
print_value(std::ostream& out, pressio_option const& opt)
{
  switch (opt.type()) {
    case pressio_option_charptr_type:
      return out << '"' << opt.get_value<std::string>() << '"';
    case pressio_option_float_type:
      return out << opt.get_value<float>();
    case pressio_option_double_type:
      return out << opt.get_value<double>();
    case pressio_option_int32_type:
      return out << opt.get_value<int>();
    case pressio_option_uint32_type:
      return out << opt.get_value<unsigned int>();
    default:
      return out;
  }
}

void
output_csv(std::ostream& out, std::string const& configuration,
           pressio_options const* options, std::vector<std::string> const& fields)
{
  out << configuration << ',';
  for (auto const& field : fields) {
    auto const& option = options->get(field);
    print_value(out, option);
    if (field == fields.back())
      out << '\n';
    else
      out << ',';
  }
  std::flush(out);
}
