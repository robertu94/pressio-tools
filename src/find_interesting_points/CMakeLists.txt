find_package(SZ REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(ZSTD IMPORTED_TARGET GLOBAL libzstd)
pkg_search_module(FFTW3 IMPORTED_TARGET GLOBAL fftw3)
add_executable(find_interesting_points find_interesting_points.cc)
target_link_libraries(find_interesting_points PRIVATE
  SZ LibPressioOpt::libpressio_opt LibDistributed::libdistributed)
