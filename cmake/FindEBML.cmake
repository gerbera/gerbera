# - Try to find libebml
# Once done this will define
#
#  EBML_FOUND - system has libebml
#  EBML_INCLUDE_DIRS - the libebml include directory
#  EBML_LIBRARIES - Link these to use EBML
#

INCLUDE(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

# EBML
pkg_search_module(PC_EBML QUIET libebml)
find_path(EBML_INCLUDE_DIR EbmlVersion.h
        HINTS ${PC_EBML_INCLUDEDIR}
        PATH_SUFFIXES ebml)
FIND_LIBRARY(EBML_LIBRARY ebml
        HINTS ${PC_EBML_LIBDIR})

find_package_handle_standard_args(EBML DEFAULT_MSG
        EBML_LIBRARY EBML_INCLUDE_DIR)
if (EBML_FOUND)
    set(EBML_LIBRARIES ${EBML_LIBRARY})
    set(EBML_INCLUDE_DIRS ${EBML_INCLUDE_DIR})
endif ()
MARK_AS_ADVANCED(
        EBML_INCLUDE_DIR
        EBML_LIBRARY
)