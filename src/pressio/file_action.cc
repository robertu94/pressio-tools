#include "file_action.h"
#include <libpressio_ext/io/hdf5.h>
#include <libpressio_ext/io/posix.h>

using namespace std::literals;

FileAction to_hdf(std::string const& filename, std::string const& dataset) {
  return [filename, dataset](pressio_data* file) {
    auto rc = pressio_io_data_path_h5write(file, filename.c_str(), dataset.c_str());
    if (rc) return ActionResult();
    else return ActionResult("failed to save HDF file "s + filename + " dataset " + dataset);
  };
}

FileAction to_binary(std::string const& filename) {
  return [filename](pressio_data* file) {
    auto rc = pressio_io_data_path_write(file, filename.c_str());
    if (rc) return ActionResult();
    else return ActionResult("failed to save HDF file "s + filename);
  };
}

FileAction noop()  {
  return [](pressio_data*){ return ActionResult(); };
}
