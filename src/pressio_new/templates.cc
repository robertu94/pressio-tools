#include "templates.h"
const char* compressor_template = R"cpp(
#include "std_compat/memory.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/domain_manager.h"

namespace libpressio { namespace $1_ns {

class $1_compressor_plugin : public libpressio_compressor_plugin {
public:
  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    return options;
  }

  struct pressio_options get_configuration_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(options, "pressio:stability", "experimental");
    std::vector<std::string> invalidations {}; 
    std::vector<pressio_configurable const*> invalidation_children {}; 
    set(options, "predictors:error_dependent", get_accumulate_configuration("predictors:error_dependent", invalidation_children, invalidations));
    set(options, "predictors:error_agnostic", get_accumulate_configuration("predictors:error_agnostic", invalidation_children, invalidations));
    set(options, "predictors:runtime", get_accumulate_configuration("predictors:runtime", invalidation_children, invalidations));
    set(options, "pressio:highlevel", get_accumulate_configuration("pressio:highlevel", invalidation_children, std::vector<std::string>{}));
    return options;
  }

  struct pressio_options get_documentation_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:description", R"()");
    return options;
  }


  int set_options_impl(struct pressio_options const& options) override
  {
    return 0;
  }

  int compress_impl(const pressio_data* real_input,
                    struct pressio_data* output) override
  {
    auto input = domain_manager().make_readable(domain_plugins().build("malloc"), *input);
    *output = std::move(input);
    return 0;
  }

  int decompress_impl(const pressio_data* real_input,
                      struct pressio_data* output) override
  {
    auto input = domain_manager().make_readable(domain_plugins().build("malloc"), *real_input);
    *output = std::move(input);
    return 0;
  }

  int major_version() const override { return 0; }
  int minor_version() const override { return 0; }
  int patch_version() const override { return 1; }
  const char* version() const override { return "0.0.1"; }
  const char* prefix() const override { return "$1"; }

  pressio_options get_metrics_results_impl() const override {
    return {};
  }

  std::shared_ptr<libpressio_compressor_plugin> clone() override
  {
    return compat::make_unique<$1_compressor_plugin>(*this);
  }

};

static pressio_register compressor_many_fields_plugin(compressor_plugins(), "$1", []() {
  return compat::make_unique<$1_compressor_plugin>();
});

} }
)cpp";

const char* metric_template = R"cpp(
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/domain_manager.h"
#include "std_compat/memory.h"

namespace libpressio { namespace $1_metrics_ns {

class $1_plugin : public libpressio_metrics_plugin {
  public:
    int end_compress_impl(struct pressio_data const* real_input, pressio_data const* real_output, int) override {
      auto input = domain_manager().make_readable(domain_plugins().build("malloc"), *real_input);
      auto output = domain_manager().make_readable(domain_plugins().build("malloc"), *real_output);
      return 0;
    }

    int end_decompress_impl(struct pressio_data const* real_input, pressio_data const* real_output, int) override {
      auto input = domain_manager().make_readable(domain_plugins().build("malloc"), *real_input);
      auto output = domain_manager().make_readable(domain_plugins().build("malloc"), *real_output);
      return 0;
    }

    int end_compress_many_impl(compat::span<const pressio_data* const> const& inputs,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
      return 0;
  }

  int end_decompress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
    return 0;
  }

  
  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "pressio:stability", "experimental");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(opts, "predictors:requires_decompress", true);
    set(opts, "predictors:invalidate", std::vector<std::string>{"predictors:error_dependent"});
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", R"()");
    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<$1_plugin>(*this);
  }
  const char* prefix() const override {
    return "$1";
  }

  private:

};

static pressio_register metrics_$1_plugin(metrics_plugins(), "$1", [](){ return compat::make_unique<$1_plugin>(); });
}}
)cpp";

const char* io_template = R"cpp(
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/io.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/domain_manager.h"
#include "std_compat/memory.h"

namespace libpressio { namespace $1_io_ns {

class $1_plugin : public libpressio_io_plugin {
  public:
  virtual struct pressio_data* read_impl(struct pressio_data* buf) override {
        return nullptr;
    }
  virtual int write_impl(struct pressio_data const* data) override{
        pressio_data input = domain_manager().make_readable(domain_plugins().build("malloc"), *data);
        return 0;
    }

  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    return options;
  }

  int set_options_impl(struct pressio_options const& options) override
  {
    return 0;
  }

  const char* version() const override { return "0.0.1"; }
  int major_version() const override { return 0; }
  int minor_version() const override { return 0; }
  int patch_version() const override { return 1; }

  
  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "pressio:stability", "experimental");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", "");
    return opt;
  }

  std::shared_ptr<libpressio_io_plugin> clone() override {
    return compat::make_unique<$1_plugin>(*this);
  }
  const char* prefix() const override {
    return "$1";
  }

  private:

};

static pressio_register io_$1_plugin(io_plugins(), "$1", [](){ return compat::make_unique<$1_plugin>(); });
}}
)cpp";

const char* search_template = R"cpp(
#include "pressio_search.h"
#include "pressio_search_results.h"
#include <std_compat/memory.h>

struct $1_search: public pressio_search_plugin {
  public:
    pressio_search_results search(compat::span<const pressio_data *const> const &input_datas,
                                  std::function<pressio_search_results::output_type(
                                          pressio_search_results::input_type const &)> compress_fn,
                                  distributed::queue::StopToken &) override {
      pressio_search_results results{};
      return results;
    }

    //configuration
    pressio_options get_options() const override {
      pressio_options opts;
      return opts;
    }

    int set_options(pressio_options const& options) override {
      return 0;
    }
    
    const char* prefix() const override {
      return "$1";
    }
    const char* version() const override {
      return "0.0.1";
    }
    int major_version() const override { return 0; }
    int minor_version() const override { return 0; }
    int patch_version() const override { return 1; }

    std::shared_ptr<pressio_search_plugin> clone() override {
      return compat::make_unique<$1_search>(*this);
    }
private:
};


static pressio_register $1_register(search_plugins(), "$1", [](){ return compat::make_unique<$1_search>();});

)cpp";
const char* searchmetric_template = R"cpp(
#include <iostream>
#include <iterator>
#include <string>
#include "pressio_search_metrics.h"
#include "pressio_search_results.h"
#include <std_compat/memory.h>

struct $1_search_metric : public pressio_search_metrics_plugin {

  $1_search_metric() {
  }

  void begin_search() override {
  }

  void end_iter(pressio_search_results::input_type const& input, pressio_search_results::output_type const& out) override {
  }

  void end_search(pressio_search_results::input_type const& input, pressio_search_results::output_type const& out) override {
  }

  virtual pressio_options get_metrics_results() override {
    return pressio_options();
  }

  const char* prefix() const override {
    return "$1";
  }

  std::shared_ptr<pressio_search_metrics_plugin> clone() override {
    return compat::make_unique<$1_search_metric>(*this);
  }
};

static pressio_register X_$1(search_metrics_plugins(), "$1", [](){ return compat::make_unique<$1_search_metric>();});
)cpp";


const char* dataset_template = R"cpp(
#include <libpressio_dataset_ext/loader.h>
#include <std_compat/memory.h>
#include <sstream>
namespace libpressio_dataset { namespace $1_loader_ns {
  struct $1_loader: public dataset_loader_base {

    size_t num_datasets_impl() override {
      return 1;
    }

    int set_options_impl(pressio_options const& options) override {
      return 0;
    }

    pressio_options get_options_impl() const override {
      pressio_options options;
      return options;
    }

    pressio_options get_documentation_impl() const override {
      pressio_options options;
      return options;
    }
    
    pressio_data load_data_impl(size_t n) override {
      return {};
    }

    pressio_options load_metadata_impl(size_t n) override {
      return {};
    }

    std::unique_ptr<dataset_loader> clone() override {
            return std::make_unique<$1_loader>(*this);
    }

    const char* prefix() const override {
      return "$1";
    }
    const char* version() const override{
      static std::string s = [this]{
        std::stringstream ss;
        ss << this->major_version() << '.';
        ss << this->minor_version() << '.';
        ss << this->patch_version();
        return ss.str();
      }();
      return s.c_str();
    }

  };

  pressio_register $1_loader_register(dataset_loader_plugins(), "$1", []{ return compat::make_unique<$1_loader>(); });
}}
)cpp";

const char* cpp_client = R"cpp(
#include <libpressio_ext/cpp/libpressio.h>
#include <libpressio_meta.h>
#include <iostream>
#include <string>

using namespace std::string_literals;

int main() {
	pressio library;
    libpressio_register_all();
	pressio_compressor compressor = library.get_compressor("pressio");

	pressio_dtype type = pressio_float_dtype;
	std::vector<size_t> dims {500,500,100};
	pressio_data metadata = pressio_data::owning(type, dims);

    pressio_io io = library.get_io("posix");
    if(!io) {
        std::cerr << library.err_msg() << std::endl;
        exit(library.err_code());
    }
    if(io->set_options({
      {"io:path", "$2"s},
    })) {
        std::cerr << io->error_msg() << std::endl;
        exit(io->error_code());
    }
	pressio_data* input = io->read(&metadata);
	pressio_data compressed = pressio_data::empty(pressio_byte_dtype, {});
	pressio_data output = pressio_data::owning(type, dims);

    if(compressor->set_options({
        {"pressio:compressor", "$1"},
        {"pressio:abs", 1e-5}
    })) {
        std::cerr << io->error_msg() << std::endl;
        exit(io->error_code());
    }

	if(compressor->compress(input, &compressed) != 0) {
		std::cerr << compressor->error_msg() << std::endl;
		exit(compressor->error_code());
	}
	if(compressor->decompress(&compressed, &output) != 0) {
		std::cerr << compressor->error_msg() << std::endl;
		exit(compressor->error_code());
	}

  std::cout << compressor->get_metrics_results() << std::endl;
	delete input;
}
)cpp";

const char* c_client = R"cpp(
#include <stdlib.h>
#include <stdio.h>
#include <libpressio.h>
#include <libpressio_meta.h>

int main() {
	struct pressio* library = pressio_instance();
    libpressio_register_all();

	struct pressio_compressor* compressor = pressio_get_compressor(library, "pressio");
    if(!compressor) {
        fprintf(stderr, "%s\n", pressio_error_msg(library));
        exit(pressio_error_code(library));
    }

	enum pressio_dtype type = pressio_float_dtype;
	size_t dims[] = {500,500,100};
	struct pressio_data* metadata = pressio_data_new_owning(type, sizeof(dims)/sizeof(dims[0]), dims);

    struct pressio_io* io = pressio_get_io(library, "posix");
    if(!io) {
        fprintf(stderr, "%s\n", pressio_error_msg(library));
        exit(pressio_error_code(library));
    }
    struct pressio_options* io_opts = pressio_options_new();
    pressio_options_set_string(io_opts, "io:path", "$2");
    if(pressio_io_set_options(io, io_opts)) {
        fprintf(stderr, "%s\n", pressio_io_error_msg(io));
        exit(pressio_io_error_code(io));
    }
    pressio_options_free(io_opts);

    struct pressio_data* input = pressio_io_read(io,metadata);
    if(!input) {
        fprintf(stderr, "%s\n", pressio_io_error_msg(io));
        exit(pressio_io_error_code(io));
    }
    pressio_data_free(metadata);
    pressio_io_free(io);
	struct pressio_data* compressed = pressio_data_new_empty(pressio_byte_dtype, 0, NULL);
	struct pressio_data* output = pressio_data_new_owning(type, sizeof(dims)/sizeof(dims[0]), dims);

    struct pressio_options* compressor_opts = pressio_options_new();
    pressio_options_set_string(compressor_opts, "pressio:compressor", "$1");
    pressio_options_set_double(compressor_opts, "pressio:abs", 1e-5);
    if(pressio_compressor_set_options(compressor, compressor_opts)) {
        fprintf(stderr, "%s\n", pressio_compressor_error_msg(compressor));
        exit(pressio_compressor_error_code(compressor));
    }
    pressio_options_free(compressor_opts);

	if(pressio_compressor_compress(compressor, input, compressed) != 0) {
        fprintf(stderr, "%s\n", pressio_compressor_error_msg(compressor));
        exit(pressio_compressor_error_code(compressor));
	}
	if(pressio_compressor_decompress(compressor,compressed,output) != 0) {
        fprintf(stderr, "%s\n", pressio_compressor_error_msg(compressor));
        exit(pressio_compressor_error_code(compressor));
	}

    struct pressio_options* metrics = pressio_compressor_get_metrics_results(compressor);
    char* metrics_str = pressio_options_to_string(metrics);
    fprintf(stdout, "%s\n", metrics_str);
    free(metrics_str);
    pressio_options_free(metrics);

	pressio_data_free(input);
	pressio_data_free(compressed);
	pressio_data_free(output);
    pressio_compressor_release(compressor);
    pressio_release(library);
}

)cpp";

const char* python_client = R"python(
import libpressio as lp
import numpy as np
from pprint import pprint

input = np.fromfile("$2", dtype=np.float32).reshape(100, 500, 500)
compressor = lp.PressioCompressor.from_config({
    "compressor_id": "pressio",
    "early_config": {
        "pressio:compressor": "$1"
    },
    "compressor_config": {
        "pressio:abs": 1e-5
    }
})
decompressed = input.copy()
compressed = compressor.encode(input)
decompressed = compressor.decode(compressed, decompressed)
pprint(compressor.get_metrics())

)python";
