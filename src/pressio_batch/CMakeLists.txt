add_executable(pressio_batch 
  pressio_batch.cc 
  datasets.cc
  compressor_configs.cc
  io.cc
  cmdline.cc
  metrics.cc
)
target_link_libraries(pressio_batch Boost::headers LibPressio::libpressio utils)

add_executable(pressio_batch_mpi
  pressio_batch_mpi.cc 
  datasets.cc
  compressor_configs.cc
  io.cc
  cmdline.cc
  metrics.cc
)
target_link_libraries(pressio_batch_mpi Boost::headers LibPressio::libpressio
  utils LibDistributed::libdistributed)

install(TARGETS pressio_batch
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
install(TARGETS pressio_batch_mpi
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
