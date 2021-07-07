add_executable(pressio pressio.cc cmdline.cc)
option(LIBPRESSIO_TOOLS_HAS_MPI "require and link to the MPI module" OFF)
if(LIBPRESSIO_TOOLS_HAS_MPI)
	target_link_libraries(pressio PRIVATE MPI::MPI_CXX utils)
endif()


install(TARGETS pressio
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
