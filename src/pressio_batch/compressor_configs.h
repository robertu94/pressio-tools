#include <memory>
#include <vector>
#include <string>

struct pressio;
struct pressio_compressor;

struct compressor_config {
  virtual ~compressor_config()=default;
  virtual pressio_compressor* load(pressio* library)=0;
  virtual std::string const& get_name()=0;
};

std::vector<std::unique_ptr<compressor_config>> load_compressors(std::string const& dataset_config_path);
