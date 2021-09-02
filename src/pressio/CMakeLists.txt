add_executable(pressio pressio.cc cmdline.cc)
target_link_libraries(pressio PRIVATE utils LibPressio::libpressio ${CMAKE_DL_LIBS})

if(LIBPRESSIO_TOOLS_HAS_MPI)
  target_link_libraries(pressio PRIVATE MPI::MPI_CXX LibDistributed::libdistributed)
endif()

install(TARGETS pressio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
