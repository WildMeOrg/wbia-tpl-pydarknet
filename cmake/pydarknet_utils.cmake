macro(GET_OS_INFO)
  string(REGEX MATCH "Linux" OS_IS_LINUX ${CMAKE_SYSTEM_NAME})
  string(REGEX MATCH "Darwin" OS_IS_MACOS ${CMAKE_SYSTEM_NAME})
  set(PYDARKNET_LIB_INSTALL_DIR "lib${LIB_SUFFIX}")
  set(PYDARKNET_INCLUDE_INSTALL_DIR
      "include/${PROJECT_NAME_LOWER}-${PYDARKNET_VERSION_MAJOR}.${PYDARKNET_VERSION_MINOR}"
  )
endmacro(GET_OS_INFO)

macro(DISSECT_VERSION)
  # Find version components
  message(STATUS "PYDARKNET_VERSION = ${PYDARKNET_VERSION}")
  string(REGEX REPLACE "^([0-9]+).*" "\\1" PYDARKNET_VERSION_MAJOR
                       "${PYDARKNET_VERSION}")
  string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" PYDARKNET_VERSION_MINOR
                       "${PYDARKNET_VERSION}")
  string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1"
                       PYDARKNET_VERSION_PATCH ${PYDARKNET_VERSION})
  string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1"
                       PYDARKNET_VERSION_CANDIDATE ${PYDARKNET_VERSION})
  set(PYDARKNET_SOVERSION
      "${PYDARKNET_VERSION_MAJOR}.${PYDARKNET_VERSION_MINOR}")
  message(STATUS "PYDARKNET_SOVERSION = ${PYDARKNET_SOVERSION}")
endmacro(DISSECT_VERSION)

macro(pydarknet_add_pyunit file)
  # find test file
  set(_file_name _file_name-NOTFOUND)
  find_file(_file_name ${file} ${CMAKE_CURRENT_SOURCE_DIR})
  if(NOT _file_name)
    message(FATAL_ERROR "Can't find pyunit file \"${file}\"")
  endif(NOT _file_name)

  # add target for running test
  string(REPLACE "/" "_" _testname ${file})
  add_custom_target(
    pyunit_${_testname}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/bin/run_test.py
            ${_file_name}
    DEPENDS ${_file_name}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
    VERBATIM
    COMMENT "Running pyunit test(s) ${file}")
  # add dependency to 'test' target
  add_dependencies(pyunit_${_testname} pydarknet)
  add_dependencies(test pyunit_${_testname})
endmacro(pydarknet_add_pyunit)
