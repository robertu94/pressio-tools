#include "datasets.h"

#include <iostream>
#include <libpressio.h>
#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/options.h>
#include <libpressio_ext/cpp/io.h>
#include <libpressio_ext/io/pressio_io.h>
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

struct generic_dataset_t: public dataset {
  generic_dataset_t(std::string const& name): dataset(name) {}
  pressio_io io;
  std::vector<size_t> dims;
  pressio_dtype type;

  pressio_data* load() override {
    pressio_data* desc = (dims.empty())
                           ? nullptr
                           : pressio_data_new_empty(type, dims.size(), dims.data());
    return pressio_io_read(&io, desc);
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


std::vector<std::unique_ptr<dataset>> load_datasets(std::string const& dataset_config_path, bool verbose) {
  pressio library;
  std::vector<std::unique_ptr<dataset>> datasets;
  pt::ptree dataset_tree;
  pt::read_json(dataset_config_path, dataset_tree);
  for (auto& [name, dataset_config] : dataset_tree) {
    if(verbose) std::cerr << "loading dataset " << name << std::endl;
    auto type = dataset_config.get<std::string>("type");
    auto io_dataset = std::make_unique<generic_dataset_t>(name);
    io_dataset->io = library.get_io(type);
    if(not io_dataset->io) throw std::runtime_error("unknown format: "s + library.err_msg());
    if(dataset_config.find("dims") != dataset_config.not_found()) {
      for (auto const& dim : dataset_config.get_child("dims")) {
        io_dataset->dims.push_back(dim.second.get_value<int>());
      }
    }
    if(dataset_config.find("dtype") != dataset_config.not_found()) {
      io_dataset->type = to_pressio_dtype(dataset_config.get<std::string>("dtype"));
    }
    if(dataset_config.find("config") != dataset_config.not_found()) {
      auto options = io_dataset->io->get_options();
      for (auto const& config : dataset_config.get_child("config")) {
        options.cast_set(config.first, config.second.get_value<std::string>(), pressio_conversion_special);
      }
      io_dataset->io->set_options(options);
    }
    datasets.emplace_back(std::move(io_dataset));
  }
  return datasets;
};
