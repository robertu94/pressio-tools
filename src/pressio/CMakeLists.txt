add_executable(pressio pressio.cc cmdline.cc)
target_link_libraries(pressio LibPressio::libpressio MPI::MPI_CXX
  LibPressioOpt::libpressio_opt utils)

install(TARGETS pressio
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
