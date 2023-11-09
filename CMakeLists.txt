cmake_minimum_required(VERSION 3.13)
project("pressio_tools" VERSION 0.4.4)

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

if(NOT USE_SANITIZER)
  set(USE_SANITIZER "" CACHE STRING "use a build sanitizer" FORCE)
endif()
set_property(CACHE USE_SANITIZER PROPERTY STRINGS "" "Address" "Thread" "Undefined" "Leak" "Memory")
if (USE_SANITIZER STREQUAL "Address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif (USE_SANITIZER STREQUAL "Thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
elseif (USE_SANITIZER STREQUAL "Undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif (USE_SANITIZER STREQUAL "Leak")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
elseif (USE_SANITIZER STREQUAL "Memory")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_SHARED_LIBS "prefer shared libraries" ON)

find_package(Boost REQUIRED)
find_package(LibPressio REQUIRED)
find_package(std_compat REQUIRED)

add_executable(pressio_new src/pressio_new/pressio_new.cc src/pressio_new/templates.cc src/pressio_new/templates.h)
target_compile_features(pressio_new PRIVATE cxx_std_17)


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
option(LIBPRESSIO_TOOLS_HAS_CUSZP "require and link to the LibPressioCuSZp module" OFF)
if(LIBPRESSIO_TOOLS_HAS_CUSZP)
  find_package(cuSZp REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC cuSZp::libpressio_cuszp)
endif()

option(LIBPRESSIO_TOOLS_HAS_LC "require and link to the LC Compressor Framework module" OFF)
if(LIBPRESSIO_TOOLS_HAS_LC)
  find_package(LC REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC LC::libpressio_lc)
endif()
option(LIBPRESSIO_TOOLS_HAS_MPI "require and link to the MPI module" OFF)
if(LIBPRESSIO_TOOLS_HAS_MPI)
  find_package(LibDistributed REQUIRED)
	find_package(MPI REQUIRED COMPONENTS CXX)
endif()
option(LIBPRESSIO_TOOLS_HAS_ADIOS1 "require and link to the adios2 module" OFF)
if(LIBPRESSIO_TOOLS_HAS_ADIOS1)
  find_package(libpressio_adios1 REQUIRED)
  target_link_libraries(libpressio_meta PUBLIC libpressio_adios1::libpressio_adios1)
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

install(TARGETS pressio_new libpressio_meta libpressio_tools_utils EXPORT LibPressioToolsTargets
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
