add_executable(pressio_batch 
  pressio_batch.cc 
  datasets.cc
  compressor_configs.cc
)
target_link_libraries(pressio_batch Boost::headers LibPressio::libpressio utils)

install(TARGETS pressio_batch)
