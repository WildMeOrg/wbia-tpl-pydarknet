# Used to discover the project's version using the in-code version definition.
#
# To use this simply add the following lines to the top of your CMakeLists.txt
# before calling `project()`:
#
#   include(cmake/VersionDiscovery.cmake)
#   discover_version()


macro(discover_version)
  # Sets the `DISCOVERED_VERSION` variable.
  # The intended use is then to use with `project(... VERSION "${DISCOVERED_VERSION}" ...)`
  #
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" setup.py --version
    RESULT_VARIABLE _exitcode
    OUTPUT_VARIABLE _output
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  if(NOT ${_exitcode} EQUAL 0)
    message(ERROR "Failed when running python setup.py --version")
    message(FATAL_ERROR "Python failed with error code: ${_exitcode}")
  endif()
  # Remove supurflous newlines (artifacts of print)
  string(STRIP "${_output}" _output)
  set(DISCOVERED_VERSION "${_output}")
endmacro()
