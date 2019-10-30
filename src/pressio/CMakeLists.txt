add_executable(pressio pressio.cc cmdline.cc file_action.cc)
target_link_libraries(pressio LibPressio::libpressio utils)
