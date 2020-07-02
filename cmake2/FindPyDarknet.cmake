# ##############################################################################
# Find PyDarknet
#
# This sets the following variables: PYDARKNET_FOUND - True if pydarknet was
# found. PYDARKNET_INCLUDE_DIRS - Directories containing the pydarknet include
# files. PYDARKNET_LIBRARIES - Libraries needed to use pydarknet.
# PYDARKNET_DEFINITIONS - Compiler flags for pydarknet.

find_package(PkgConfig)
pkg_check_modules(PC_PYDARKNET pydarknet)
set(PYDARKNET_DEFINITIONS ${PC_PYDARKNET_CFLAGS_OTHER})

find_path(PYDARKNET_INCLUDE_DIR pydarknet/pydarknet.hpp
          HINTS ${PC_PYDARKNET_INCLUDEDIR} ${PC_PYDARKNET_INCLUDE_DIRS})

find_library(PYDARKNET_LIBRARY pydarknet HINTS ${PC_PYDARKNET_LIBDIR}
                                               ${PC_PYDARKNET_LIBRARY_DIRS})

set(PYDARKNET_INCLUDE_DIRS ${PYDARKNET_INCLUDE_DIR})
set(PYDARKNET_LIBRARIES ${PYDARKNET_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PyDarknet DEFAULT_MSG PYDARKNET_LIBRARY
                                  PYDARKNET_INCLUDE_DIR)

mark_as_advanced(PYDARKNET_LIBRARY PYDARKNET_INCLUDE_DIR)
