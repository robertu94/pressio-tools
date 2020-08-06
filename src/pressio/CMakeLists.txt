add_executable(pressio pressio.cc cmdline.cc)
target_link_libraries(pressio PRIVATE MPI::MPI_CXX utils)

install(TARGETS pressio
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
