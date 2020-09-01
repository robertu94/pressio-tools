cmake_minimum_required(VERSION 3.13)
project("pressio_tools" VERSION 0.0.4)

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
find_package(MPI REQUIRED COMPONENTS CXX)
find_package(LibDistributed REQUIRED)

add_library(utils 
  src/utils/string_options.cc
  )
target_link_libraries(utils PUBLIC LibPressio::libpressio LibDistributed::libdistributed)
target_include_directories(utils PUBLIC include/)

option(LIBPRESSIO_TOOLS_HAS_OPT "require and link to the LibPressioOpt module" OFF)
if(LIBPRESSIO_TOOLS_HAS_OPT)
  find_package(LibPressioOpt REQUIRED)
  target_link_libraries(utils PUBLIC LibPressioOpt::libpressio_opt)
  add_subdirectory(src/find_interesting_points/)
endif()
option(LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR "require and link to the LibPressioErrorInjector module" OFF)
if(LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR)
  find_package(LibPressioErrorInjector REQUIRED)
  target_link_libraries(utils PUBLIC LibPressioErrorInjector::libpressio_error_injector)
endif()

add_subdirectory(src/pressio)
add_subdirectory(src/pressio_batch)
add_subdirectory(test)
