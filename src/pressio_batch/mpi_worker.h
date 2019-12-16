#pragma once
#include <vector>
#include <memory>
#include "datasets.h"
#include "compressor_configs.h"

struct pressio_metrics;
void worker(std::vector<std::unique_ptr<dataset>> const& datasets,
            std::vector<std::unique_ptr<compressor_config>> const& compressors,
            pressio* library,
            pressio_metrics* metrics,
            std::vector<std::string> const& fields);
