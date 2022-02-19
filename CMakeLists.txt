cmake_minimum_required(VERSION 3.13)
project("pressio_tools" VERSION 0.0.24)

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

find_package(Boost REQUIRED)
find_package(LibPressio REQUIRED)
find_package(std_compat REQUIRED)

add_library(utils 
  src/utils/string_options.cc
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/pressio_tools_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/utils/pressio_tools_version.h
  )
target_link_libraries(utils PUBLIC LibPressio::libpressio)
target_include_directories(utils PUBLIC 
  include/
  ${CMAKE_CURRENT_BINARY_DIR}/src/
  )

option(LIBPRESSIO_TOOLS_HAS_RMETRIC "require and link to the libpressio R metrics module" OFF)
if(LIBPRESSIO_TOOLS_HAS_RMETRIC)
  find_package(libpressio_r_metric REQUIRED)
  target_link_libraries(utils PUBLIC libpressio_r_metric::libpressio_r_metric)
endif()
option(LIBPRESSIO_TOOLS_HAS_TTHRESH "require and link to the libpressio tthresh module" OFF)
if(LIBPRESSIO_TOOLS_HAS_TTHRESH)
  find_package(PressioTThresh REQUIRED)
  target_link_libraries(utils PUBLIC PressioTThresh::libpressio_tthresh)
endif()
option(LIBPRESSIO_TOOLS_HAS_OPT "require and link to the LibPressioOpt module" OFF)
if(LIBPRESSIO_TOOLS_HAS_OPT)
  find_package(LibPressioOpt REQUIRED)
  target_link_libraries(utils PUBLIC LibPressioOpt::libpressio_opt)
endif()
option(LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR "require and link to the LibPressioErrorInjector module" OFF)
if(LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR)
  find_package(LibPressioErrorInjector REQUIRED)
  target_link_libraries(utils PUBLIC LibPressioErrorInjector::libpressio_error_injector)
endif()
option(LIBPRESSIO_TOOLS_HAS_MPI "require and link to the MPI module" OFF)
if(LIBPRESSIO_TOOLS_HAS_MPI)
  find_package(LibDistributed REQUIRED)
	find_package(MPI REQUIRED COMPONENTS CXX)
endif()
add_subdirectory(src/pressio)
add_subdirectory(src/pressio_batch)

if(BUILD_TESTING)
add_subdirectory(test)
endif()
