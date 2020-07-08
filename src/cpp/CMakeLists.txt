# include_directories(${CMAKE_SOURCE_DIR}/include algorithms ext util nn .)

message(STATUS "PYDARKNET_VERSION = ${PYDARKNET_VERSION}")
set(PYDARKNET_VERSION ${PYDARKNET_VERSION})
message(STATUS "PYDARKNET_VERSION = ${PYDARKNET_VERSION}")
add_definitions(-D_PYDARKNET_VERSION=${PYDARKNET_VERSION})
add_definitions(-DPYDARKNET_VERSION_=${PYDARKNET_VERSION})
add_definitions(-DPYDARKNET_VERSION=${PYDARKNET_VERSION})
add_definitions(-D_PYDARKNET_VERSION=${PYDARKNET_VERSION})
add_definitions(-DPYDARKNET_VERSION_=${PYDARKNET_VERSION})
add_definitions(-DPYDARKNET_VERSION=${PYDARKNET_VERSION})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pydarknet/config.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/pydarknet/config.h)

file(GLOB C_SOURCES "pydarknet/*.c")
file(GLOB HEADER_SOURCES "pydarknet/*.h")
file(GLOB CU_SOURCES "pydarknet/*.cu")

add_library(pydarknet_s STATIC "${C_SOURCES};${HEADER_SOURCES}")

target_link_libraries(pydarknet_s ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT})

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
  set_target_properties(pydarknet_s PROPERTIES COMPILE_FLAGS -fPIC)
endif()

set_property(TARGET pydarknet_s PROPERTY COMPILE_DEFINITIONS PYDARKNET_STATIC)

if(BUILD_CUDA_LIB)
  set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};-DPYDARKNET_USE_CUDA")
  # set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};--gpu-architecture=compute_50;--gpu-
  # code=compute_50;--compiler-options;${CFLAGS}") set(CUDA_NVCC_FLAGS
  # "${CUDA_NVCC_FLAGS};--gpu-architecture=compute_50;--gpu-
  # code=compute_50;--compiler-options")

  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};-Xcompiler")
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};-fPIC")
    if(NVCC_COMPILER_BINDIR)
      set(CUDA_NVCC_FLAGS
          "${CUDA_NVCC_FLAGS};--compiler-bindir=${NVCC_COMPILER_BINDIR}")
    endif()
  else()
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};")
  endif()

  cuda_add_library(pydarknet_cuda_s STATIC
                   "${C_SOURCES};${HEADER_SOURCES};${CU_SOURCES}")

  target_link_libraries(
    pydarknet_cuda_s ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${CUDA_LIBRARIES}
    ${CUDA_CUBLAS_LIBRARIES} ${CUDA_curand_LIBRARY})

  set_property(TARGET pydarknet_cuda_s PROPERTY COMPILE_DEFINITIONS
                                                PYDARKNET_STATIC)
  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set_target_properties(pydarknet_cuda_s PROPERTIES COMPILE_FLAGS -fPIC)
  endif()
endif()

add_library(pydarknet SHARED "${HEADER_SOURCES}")

if(BUILD_CUDA_LIB)
  cuda_add_library(pydarknet_cuda SHARED "${HEADER_SOURCES}")
endif()

if(OS_IS_LINUX)
  set(LINK_FLAG_PREFIX "-Wl,--whole-archive")
  set(LINK_FLAG_POSTFIX "-Wl,--no-whole-archive")

  set_target_properties(pydarknet PROPERTIES LINKER_LANGUAGE CXX)

  if(BUILD_CUDA_LIB)
    set_target_properties(pydarknet_cuda PROPERTIES LINKER_LANGUAGE CXX)
  endif()
elseif(OS_IS_MACOS)
  set(LINK_FLAG_PREFIX "-Wl,-force_load")

  set_target_properties(pydarknet PROPERTIES LINKER_LANGUAGE CXX)

  if(BUILD_CUDA_LIB)
    set_target_properties(pydarknet_cuda PROPERTIES LINKER_LANGUAGE CXX)
  endif()
else()
  set(LINK_FLAG_PREFIX "/WHOLEARCHIVE")

  set_target_properties(pydarknet PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)

  if(BUILD_CUDA_LIB)
    set_target_properties(pydarknet_cuda PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS
                                                    YES)
  endif()
endif()

target_link_libraries(pydarknet ${LINK_FLAG_PREFIX} pydarknet_s
                      ${LINK_FLAG_POSTFIX})

if(BUILD_CUDA_LIB)
  target_link_libraries(pydarknet_cuda ${LINK_FLAG_PREFIX} pydarknet_cuda_s
                        ${LINK_FLAG_POSTFIX})
  # target_link_libraries(pydarknet_cuda cudpp_x86_64)
endif()

set_target_properties(
  pydarknet
  PROPERTIES VERSION ${PYDARKNET_VERSION} SOVERSION ${PYDARKNET_SOVERSION}
             DEFINE_SYMBOL PYDARKNET_EXPORTS)

if(BUILD_CUDA_LIB)
  set_target_properties(
    pydarknet_cuda
    PROPERTIES VERSION ${PYDARKNET_VERSION} SOVERSION ${PYDARKNET_SOVERSION}
               DEFINE_SYMBOL PYDARKNET_EXPORTS)
endif()

if(NOT SKBUILD)
  install(
    TARGETS pydarknet pydarknet_s
    EXPORT ${targets_export_name}
    INCLUDES
    DESTINATION include
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${PYDARKNET_LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${PYDARKNET_LIB_INSTALL_DIR})

  if(BUILD_CUDA_LIB)
    install(
      TARGETS pydarknet_cuda pydarknet_cuda_s
      EXPORT ${targets_export_name}
      INCLUDES
      DESTINATION include
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION ${PYDARKNET_LIB_INSTALL_DIR}
      ARCHIVE DESTINATION ${PYDARKNET_LIB_INSTALL_DIR})
  endif()

  install(
    DIRECTORY pydarknet
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")

else()
  install(
    TARGETS pydarknet pydarknet_s
    EXPORT ${targets_export_name}
    LIBRARY DESTINATION ${PYDARKNET_LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${PYDARKNET_LIB_INSTALL_DIR})

  if(BUILD_CUDA_LIB)
    install(
      TARGETS pydarknet_cuda pydarknet_cuda_s
      EXPORT ${targets_export_name}
      LIBRARY DESTINATION ${PYDARKNET_LIB_INSTALL_DIR}
      ARCHIVE DESTINATION ${PYDARKNET_LIB_INSTALL_DIR})
  endif()
endif()
