cmake_minimum_required(VERSION 3.13)
project("pressio_tools" VERSION 0.1.5)

#correct was to set a default build type
# https://blog.kitware.com/cmake-and-the-default-build-type/
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type was set. Setting build type to ${default_build_type}.")
  set(CMAKE_BUILD_TYPE ${default_build_type} CACHE 
    STRING "Choose the type to build" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

include(CTest)
include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_SHARED_LIBS "prefer shared libraries" ON)

find_package(Boost REQUIRED)
find_package(LibPressio REQUIRED)
find_package(std_compat REQUIRED)

add_library(libpressio_tools_utils 
  src/utils/string_options.cc
  )
target_link_libraries(libpressio_tools_utils PUBLIC LibPressio::libpressio)
target_include_directories(libpressio_tools_utils PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/>
  )

add_library(libpressio_meta 
  src/pressio_meta/pressio_meta.cc
  )
target_link_libraries(libpressio_meta PUBLIC LibPressio::libpressio)
target_include_directories(libpressio_meta PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
target_include_directories(libpressio_meta PRIVATE 
  ${CMAKE_CURRENT_BINARY_DIR}/src/utils/
  )

option(LIBPRESSIO_TOOLS_HAS_HLRCOMPRESS "require and link to the libpressio hlrcompress metrics module" OFF)
if(LIBPRESSIO_TOOLS_HAS_HLRCOMPRESS)
  find_package(LibPressioHLRCompress REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioHLRCompress::libpressio_hlrcompress)
endif()
option(LIBPRESSIO_TOOLS_HAS_NVCOMP "require and link to the libpressio nvcomp metrics module" OFF)
if(LIBPRESSIO_TOOLS_HAS_NVCOMP)
  find_package(LibPressioNVComp REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioNVComp::libpressio_nvcomp)
endif()
option(LIBPRESSIO_TOOLS_HAS_FRSZ "require and link to the libpressio FRFZ module" OFF)
if(LIBPRESSIO_TOOLS_HAS_FRSZ)
  find_package(frsz REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC frsz::frsz)
endif()
option(LIBPRESSIO_TOOLS_HAS_RMETRIC "require and link to the libpressio R metrics module" OFF)
if(LIBPRESSIO_TOOLS_HAS_RMETRIC)
  find_package(LibPressioRMetric REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioRMetric::libpressio_r_metric)
endif()
option(LIBPRESSIO_TOOLS_HAS_SPERR "require and link to the libpressio sperr module" OFF)
if(LIBPRESSIO_TOOLS_HAS_SPERR)
  find_package(LibPressioSperr REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioSperr::libpressio_sperr)
endif()
option(LIBPRESSIO_TOOLS_HAS_TTHRESH "require and link to the libpressio tthresh module" OFF)
if(LIBPRESSIO_TOOLS_HAS_TTHRESH)
  find_package(LibPressioTThresh REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioTThresh::libpressio_tthresh)
endif()
option(LIBPRESSIO_TOOLS_HAS_OPT "require and link to the LibPressioOpt module" OFF)
if(LIBPRESSIO_TOOLS_HAS_OPT)
  find_package(LibPressioOpt REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioOpt::libpressio_opt)
endif()
option(LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR "require and link to the LibPressioErrorInjector module" OFF)
if(LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR)
  find_package(LibPressioErrorInjector REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LibPressioErrorInjector::libpressio_error_injector)
endif()
option(LIBPRESSIO_TOOLS_HAS_MPI "require and link to the MPI module" OFF)
if(LIBPRESSIO_TOOLS_HAS_MPI)
  find_package(LibDistributed REQUIRED)
	find_package(MPI REQUIRED COMPONENTS CXX)
endif()
option(LIBPRESSIO_TOOLS_HAS_ADIOS2 "require and link to the adios2 module" OFF)
if(LIBPRESSIO_TOOLS_HAS_ADIOS2)
  find_package(libpressio_adios2 REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC libpressio_adios2::libpressio_adios2)
endif()
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/pressio_tools_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/utils/pressio_tools_version.h
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libpressio_meta.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libpressio_meta.pc
  @ONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libpressio_meta_cxx.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libpressio_meta_cxx.pc
  @ONLY
  )

add_subdirectory(src/pressio)
add_subdirectory(src/pressio_batch)

if(BUILD_TESTING)
add_subdirectory(test)
endif()

install(TARGETS libpressio_meta libpressio_tools_utils EXPORT LibPressioToolsTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
install(EXPORT LibPressioToolsTargets NAMESPACE LibPressioTools:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibPressioTools/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpressio_meta.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pkgconfig)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpressio_meta_cxx.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pkgconfig)
install(FILES include/libpressio_meta.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/LibPressioToolsConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/LibPressioToolsConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibPressioTools
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/LibPressioToolsConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LibPressioToolsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LibPressioToolsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibPressioTools
)
