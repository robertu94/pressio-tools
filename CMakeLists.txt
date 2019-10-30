cmake_minimum_required(VERSION 3.13)
project("pressio_tools" VERSION 0.0.1)

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

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LibPressio REQUIRED)
find_package(SZ REQUIRED)
find_package(zfp REQUIRED)
find_package(ZLIB REQUIRED)
find_package(MGARD REQUIRED)
find_package(PkgConfig)
pkg_search_module(ZSTD IMPORTED_TARGET GLOBAL libzstd)
pkg_search_module(FFTW3 IMPORTED_TARGET GLOBAL fftw3)

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE include/)

add_subdirectory(src/pressio)
add_subdirectory(test)
