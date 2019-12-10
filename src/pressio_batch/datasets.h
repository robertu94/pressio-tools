#include <memory>
#include <vector>
#include <string>

struct pressio_data;

struct dataset {
  dataset(std::string name): name(name) {}
  virtual ~dataset()=default;
  virtual pressio_data* load()=0;
  std::string const& get_name() {return name;}

  private:
  std::string name;
};

std::vector<std::unique_ptr<dataset>> load_datasets(std::string const& dataset_config_path);
