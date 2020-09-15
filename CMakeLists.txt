cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 11)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0042 NEW)
endif(COMMAND cmake_policy)

project(pydarknet LANGUAGES C CXX)
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

if(SKBUILD)
  find_package(PythonExtensions REQUIRED)
endif()

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

include(${PROJECT_SOURCE_DIR}/CMake/pydarknet_utils.cmake)

function(get_version outvar)
  execute_process(
    COMMAND python setup.py --version
    RESULT_VARIABLE _exitcode
    OUTPUT_VARIABLE _output
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  if(NOT ${_exitcode} EQUAL 0)
    message(ERROR "Failed when running python code: \"\"\"
python setup.py --version\"\"\"")
    message(FATAL_ERROR "Python command failed with error code: ${_exitcode}")
  endif()
  # Remove supurflous newlines (artifacts of print)
  string(STRIP "${_output}" _output)
  set(${outvar}
      "${_output}"
      PARENT_SCOPE)
endfunction()
get_version(PYDARKNET_VERSION)

message(STATUS "PYDARKNET_VERSION = ${PYDARKNET_VERSION}")

dissect_version()
get_os_info()

if(OS_IS_MACOS)
  message(STATUS "INCLUDING MACPORTS")

  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)

  find_package(Eigen3 3 REQUIRED NO_MODULE)

  set(CMAKE_INSTALL_RPATH "/opt/local/lib")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++")
endif()

# Setup basic python stuff and ensure we have skbuild list(INSERT
# CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/CMake") include( skbuild-helpers )

# ##############################################################################

# detect if using the Clang compiler
if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Add an "uninstall" target
configure_file("${PROJECT_SOURCE_DIR}/CMake/uninstall_target.cmake.in"
               "${PROJECT_BINARY_DIR}/uninstall_target.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P
                            "${PROJECT_BINARY_DIR}/uninstall_target.cmake")

# Set the build type.  Options are: Debug          : w/ debug symbols, w/o
# optimization Release        : w/o debug symbols, w/ optimization
# RelWithDebInfo : w/ debug symbols, w/ optimization MinSizeRel     : w/o debug
# symbols, w/ optimization, stripped binaries

if(NOT CMAKE_BUILD_TYPE)
  # set(CMAKE_BUILD_TYPE Release)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Build type" FORCE)
  # set(CMAKE_BUILD_TYPE Debug)
endif()

# set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
# set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
# set output path for tests
set(TEST_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/test)

option(BUILD_CUDA_LIB "Build CUDA library" ON)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DOC "Build documentation" ON)

set(NVCC_COMPILER_BINDIR
    ""
    CACHE
      PATH
      "Directory where nvcc should look for C++ compiler. This is passed to nvcc through the --compiler-bindir option."
)

if(OS_IS_MACOS)
  find_package(Gflags REQUIRED)
  if(Gflags_FOUND
     OR GFLAGS_FOUND
     OR GFLAGS_INCLUDE_DIRS)
    message(STATUS "GFLAGS_INCLUDE_DIR: ${GFLAGS_INCLUDE_DIR}")
    message(STATUS "GFLAGS_LIBRARIES: ${GFLAGS_LIBRARIES}")
    include_directories(${GFLAGS_INCLUDE_DIR})
  else()
    message(FATAL_ERROR "GFlags NOT found")
  endif()
endif()

find_package(OpenCV 3.4 REQUIRED)
# find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
  add_definitions(-DOPENCV)
  include_directories(${OpenCV_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "OpenCV NOT found")
endif()

find_package(Threads REQUIRED)
if(Threads_FOUND)
  message(STATUS "Threads found")
else()
  message(FATAL_ERROR "Threads NOT found")
endif()

# CUDA support
if(BUILD_CUDA_LIB)
  find_package(CUDA)
  if(CUDA_FOUND)
    enable_language(CUDA)
    add_definitions(-DGPU)
    message(
      STATUS
        "CUDA found (include: ${CUDA_INCLUDE_DIRS}, lib: ${CUDA_LIBRARIES})")
    include_directories(${CUDA_INCLUDE_DIRS})
  else(CUDA_FOUND)
    message(STATUS "CUDA not found, CUDA library will not be built")
    set(BUILD_CUDA_LIB OFF)
  endif(CUDA_FOUND)
endif(BUILD_CUDA_LIB)

find_package(PkgConfig REQUIRED)

# set the C/C++ include path to the "include" directory
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/src/cpp)

# require proper c++ add_definitions( "-Wall -ansi -pedantic" )
if(CMAKE_C_COMPILER_ID MATCHES "MSVC" OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # lots of warnings with cl.exe right now, use /W1
  add_definitions(
    "/W1 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /bigobj")
else()
  add_definitions("-O3 -g -Wall -Wno-unknown-pragmas -Wno-unused-function")
endif()

# install and export variables
set(config_install_dir "lib/CMake/${PROJECT_NAME}")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/pydarknet-config-version.cmake")
set(project_config "${generated_dir}/pydarknet-config.cmake")
set(targets_export_name "pydarknet-targets")
set(namespace "pydarknet::")

if(SKBUILD)
  # Override output paths when using scikit-build
  set(PYDARKNET_LIB_INSTALL_DIR "pydarknet/lib")
  set(PYDARKNET_INCLUDE_INSTALL_DIR "pydarknet/include")
endif()

if(NOT SKBUILD)
  add_subdirectory("${CMAKE_SOURCE_DIR}/CMake")
endif()
add_subdirectory("${CMAKE_SOURCE_DIR}/src")
if(BUILD_EXAMPLES)
  add_subdirectory("${CMAKE_SOURCE_DIR}/examples")
endif(BUILD_EXAMPLES)
if(BUILD_TESTS)
  add_subdirectory("${CMAKE_SOURCE_DIR}/tests")
endif(BUILD_TESTS)

# CMake configuration file creation Include module with fuction
# 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure 'pydarknet-config-version.cmake' Note: PYDARKNET_VERSION is used as
# a VERSION
write_basic_package_version_file(
  "${version_config}"
  VERSION ${PYDARKNET_VERSION}
  COMPATIBILITY SameMajorVersion)

# Configure 'pydarknet-config.cmake' Use variables: * targets_export_name *
# PROJECT_NAME
configure_package_config_file("CMake/Config.cmake.in" "${project_config}"
                              INSTALL_DESTINATION "${config_install_dir}")

if(NOT SKBUILD)
  # Config * <prefix>/lib/CMake/pydarknet/pydarknet-config.cmake *
  # <prefix>/lib/CMake/pydarknet/pydarknet-config-version.cmake
  install(FILES "${project_config}" "${version_config}"
          DESTINATION "${config_install_dir}")
  # Config * <prefix>/lib/CMake/pydarknet/pydarknet-targets.cmake
  install(
    EXPORT "${targets_export_name}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}")
endif()

# CPACK options

# RPM
find_program(RPM_PROGRAM rpm)
if(EXISTS ${RPM_PROGRAM})
  list(APPEND CPACK_GENERATOR "RPM")
endif(EXISTS ${RPM_PROGRAM})
# DEB
find_program(DPKG_PROGRAM dpkg)
if(EXISTS ${DPKG_PROGRAM})
  list(APPEND CPACK_GENERATOR "DEB")
endif(EXISTS ${DPKG_PROGRAM})
# NSIS
find_program(NSIS_PROGRAM makensis MakeNSIS)
if(EXISTS ${NSIS_PROGRAM})
  list(APPEND CPACK_GENERATOR "NSIS")
endif(EXISTS ${NSIS_PROGRAM})
# dpkg
find_program(PACKAGE_MAKER_PROGRAM PackageMaker
             HINTS /Developer/Applications/Utilities)
if(EXISTS ${PACKAGE_MAKER_PROGRAM})
  list(APPEND CPACK_GENERATOR "PackageMaker")
endif(EXISTS ${PACKAGE_MAKER_PROGRAM})

set(CPACK_GENERATOR "${CPACK_GENERATOR}")
set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_SET_DESTDIR ON)
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_CONTACT "Marius Muja")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_VERSION ${PYDARKNET_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PYDARKNET_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PYDARKNET_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PYDARKNET_VERSION_PATCH})
include(CPack)

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Building examples: ${BUILD_EXAMPLES}")
message(STATUS "Building tests: ${BUILD_TESTS}")
message(STATUS "Building documentation: ${BUILD_DOC}")
message(STATUS "Building CUDA library: ${BUILD_CUDA_LIB}")
message(STATUS "Using MPI support: ${USE_MPI}")
