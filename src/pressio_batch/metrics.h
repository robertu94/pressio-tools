#pragma once

#include <memory>
#include <string>

struct pressio;
struct pressio_metrics;

struct metrics_config {
  virtual  ~metrics_config()=default;
  virtual pressio_metrics* load(pressio* library)=0;
};

std::unique_ptr<metrics_config> load_metrics(std::string const& metrics_config_path, bool verbose = false);
