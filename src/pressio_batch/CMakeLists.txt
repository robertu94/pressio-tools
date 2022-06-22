add_executable(pressio_batch 
  pressio_batch.cc 
  datasets.cc
  compressor_configs.cc
  io.cc
  cmdline.cc
  metrics.cc
)
target_link_libraries(pressio_batch PRIVATE Boost::headers libpressio_meta libpressio_tools_utils)
install(TARGETS pressio_batch
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

option(LIBPRESSIO_TOOLS_HAS_MPI "require and link to the MPI module" OFF)
if(LIBPRESSIO_TOOLS_HAS_MPI)
  add_executable(pressio_batch_mpi
    pressio_batch_mpi.cc 
    datasets.cc
    compressor_configs.cc
    io.cc
    cmdline.cc
    metrics.cc
  )
target_link_libraries(pressio_batch_mpi Boost::headers libpressio_meta
    libpressio_tools_utils LibDistributed::libdistributed
  )
  install(TARGETS pressio_batch_mpi
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

