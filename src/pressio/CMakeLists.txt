add_executable(pressio pressio.cc cmdline.cc options.cc)
target_link_libraries(pressio PRIVATE libpressio_tools_utils libpressio_meta ${CMAKE_DL_LIBS})
target_include_directories(pressio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(LIBPRESSIO_TOOLS_HAS_MPI)
  target_link_libraries(pressio PRIVATE MPI::MPI_CXX LibDistributed::libdistributed)
endif()

install(TARGETS pressio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
