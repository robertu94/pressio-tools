add_executable(pressio pressio.cc cmdline.cc)
target_link_libraries(pressio LibPressio::libpressio utils)

install(TARGETS pressio)
