#include <functional>
#include <pressio_data.h>

struct ActionResult {
  int err_code{0};
  std::string err_msg{};
  ActionResult(): err_code(0), err_msg() {}
  ActionResult(std::string const& err_msg): err_code(1), err_msg(err_msg) {}
  operator bool() { return err_code == 0; }
};

using FileAction = std::function<ActionResult(pressio_data*)>;

FileAction noop();
FileAction to_hdf(std::string const& filename, std::string const& dataset); 
FileAction to_binary(std::string const& filename); 
