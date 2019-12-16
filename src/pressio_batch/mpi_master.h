#pragma once
#include <memory>
#include <vector>
#include "datasets.h"
#include "compressor_configs.h"

struct pressio_metrics;

void master(std::vector<std::unique_ptr<dataset>> const& datasets,
            std::vector<std::unique_ptr<compressor_config>> const& compressors,
            std::vector<std::string> fields,
            pressio_metrics* metrics
    );
