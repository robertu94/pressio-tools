#include "datasets.h"

#include <iostream>
#include <libpressio.h>
#include <libpressio_ext/io/posix.h>
#include <libpressio_ext/io/hdf5.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;
using namespace std::literals;

struct file_dataset_t: public dataset {
  file_dataset_t(std::string const& name): dataset(name) {}
  std::string filepath;
  std::vector<size_t> dims;
  pressio_dtype type;
  pressio_data* load() override {
    pressio_data* metadata = pressio_data_new_owning(type, dims.size(), dims.data());
    pressio_data* data = pressio_io_data_path_read(metadata, filepath.c_str());
    return data;
  }
};

struct hdf_dataset_t: public dataset {
  hdf_dataset_t(std::string const& name): dataset(name) {}
  std::string filepath;
  std::string dataset_name;
  pressio_data* load() override {
    return pressio_io_data_path_h5read(filepath.c_str(), dataset_name.c_str());
  }
};

pressio_dtype to_pressio_dtype(std::string const& name) {
  if(name == "float") return pressio_float_dtype;
  if(name == "double") return pressio_double_dtype;
  if(name == "int8") return pressio_int8_dtype;
  if(name == "int16") return pressio_int16_dtype;
  if(name == "int32") return pressio_int32_dtype;
  if(name == "int64") return pressio_int64_dtype;
  if(name == "uint8") return pressio_uint8_dtype;
  if(name == "uint16") return pressio_uint16_dtype;
  if(name == "uint32") return pressio_uint32_dtype;
  if(name == "uint64") return pressio_uint64_dtype;
  throw std::runtime_error("invalid datatype "s + name);
}


std::vector<std::unique_ptr<dataset>> load_datasets(std::string const& dataset_config_path) {
  std::vector<std::unique_ptr<dataset>> datasets;
  pt::ptree dataset_tree;
  pt::read_json(dataset_config_path, dataset_tree);
  for (auto& [name, dataset_config] : dataset_tree) {
    auto type = dataset_config.get<std::string>("type");
    if(type == "raw") {
      std::clog << "loading raw dataset" << name << std::endl;
      auto file_dataset = std::make_unique<file_dataset_t>(name);
      file_dataset->filepath = dataset_config.get<std::string>("path");
      file_dataset->type = to_pressio_dtype(dataset_config.get<std::string>("dtype"));
      for (auto const& dim : dataset_config.get_child("dims")) {
        file_dataset->dims.push_back(dim.second.get_value<int>());
      }
      datasets.emplace_back(std::move(file_dataset));
    } else if(type == "hdf") {
      std::clog << "loading hdf dataset" << name << std::endl;
      auto hdf_dataset = std::make_unique<hdf_dataset_t>(name);
      hdf_dataset->filepath = dataset_config.get<std::string>("path");
      hdf_dataset->dataset_name = dataset_config.get<std::string>("dataset");
      datasets.emplace_back(std::move(hdf_dataset));
    }
    else {
      throw std::runtime_error("invalid dataset configuration for "s + name);
    }
  }
  return datasets;
};
