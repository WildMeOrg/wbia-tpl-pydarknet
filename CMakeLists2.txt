cmake_minimum_required(VERSION 2.13.0)

include(cmake/VersionDiscovery.cmake)
discover_version()
# CMP0048 is used to enable `project()` to manage `VERSION` & `VERSION*` variables
cmake_policy(SET CMP0048 NEW)

#######################################

project(pydarknet VERSION "${DISCOVERED_VERSION}" LANGUAGES C CXX)
set(CMAKE_BUILD_TYPE "Release")

option(CUDA "build with cuda" Off)
set(CFLAGS -Wall -O2)


#######################################
# Find required packages

if (APPLE)
  # Include Homebrew installed libraries
  message("USING Homebrew")
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
endif()

message(STATUS "Checking OpenCV")
find_package( OpenCV REQUIRED )
add_definitions(-DOPENCV)
include_directories(${OpenCV_INCLUDE_DIRS})

message(STATUS "Checking Threads")
find_package( Threads REQUIRED )

if (CUDA)
  message(STATUS "Checking CUDA")
  find_package( CUDA REQUIRED )
  add_definitions(-DGPU)
  include_directories(${CUDA_INCLUDE_DIRS})
  list(APPEND CUDA_NVCC_FLAGS --gpu-architecture=compute_50 --gpu-code=compute_50 --compiler-options ${CFLAGS})
endif()

#######################################
# Define library targets

file(GLOB SOURCE_FILES "src/*.c")
file(GLOB HEADER_FILES "src/*.h")
file(GLOB CUDA_SOURCE_FILES "src/*.cu")


if (CUDA)
  cuda_add_library(pydarknet.gpu SHARED
    ${CUDA_SOURCE_FILES}
    ${SOURCE_FILES}
    ${HEADER_FILES}
  )

  target_link_libraries(pydarknet.gpu
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CUDA_CUBLAS_LIBRARIES}
    ${CUDA_curand_LIBRARY}
  )
  set(install_target pydarknet.gpu)
else ()
  add_library(pydarknet.cpu SHARED
    ${SOURCE_FILES}
  )

  target_link_libraries(pydarknet.cpu
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CUDA_CUBLAS_LIBRARIES}
    ${CUDA_curand_LIBRARY}
  )
  set(install_target pydarknet.cpu)
endif ()

if (SKBUILD)
  install(TARGETS ${install_target} DESTINATION pydarknet/lib)
  install(FILES ${HEADER_FILES} DESTINATION pydarknet/include)
else ()
  install(TARGETS ${install_target} DESTINATION lib)
  install(FILES ${HEADER_FILES} DESTINATION include)
endif()
