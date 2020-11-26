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
pkg_check_modules(PC_EBM QUIET libebml)
find_path(EBML_INCLUDE_DIR StdIOCallback.h
        HINTS ${PC_EBM_INCLUDEDIR} ${PC_EBM_INCLUDE_DIRS}
        PATH_SUFFIXES ebml)
FIND_LIBRARY(EBML_LIBRARY ebml
        HINTS ${PC_EBM_LIBDIR} ${PC_EBM_LIBRARY_DIRS})
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EBML
        REQUIRED_VARS EBML_LIBRARY EBML_INCLUDE_DIR)
if (EBML_FOUND)
    set(EBML_LIBRARIES ${EBML_LIBRARY} ${PC_EBM_LIBRARIES})
    set(EBML_INCLUDE_DIRS ${EBML_INCLUDE_DIR})
endif ()
MARK_AS_ADVANCED(
        EBML_INCLUDE_DIR
        EBML_LIBRARY
)