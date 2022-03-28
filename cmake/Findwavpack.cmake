# - Try to find wavpack
# Once done this will define
#
#  WAVPACK_FOUND - system has wavpack
#  wavpack_INCLUDE_DIRS - the wavpack include directory
#  wavpack_LIBRARIES - Link these to use WAVPACK
#

INCLUDE(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_WAVPACK QUIET wavpack)

find_path(WAVPACK_INCLUDE_DIR wavpack.h
    HINTS ${PC_WAVPACK_INCLUDEDIR} ${PC_WAVPACK_INCLUDE_DIRS}
    PATH_SUFFIXES wavpack)

FIND_LIBRARY(WAVPACK_LIBRARY wavpack
    HINTS ${PC_WAVPACK_LIBDIR} ${PC_WAVPACK_LIBRARY_DIRS})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(wavpack
    REQUIRED_VARS WAVPACK_LIBRARY WAVPACK_INCLUDE_DIR)

if (WAVPACK_FOUND)
    set (wavpack_LIBRARIES ${WAVPACK_LIBRARY} ${PC_WAVPACK_LIBRARIES})
    set (wavpack_INCLUDE_DIRS ${WAVPACK_INCLUDE_DIR} )

    if(NOT TARGET wavpack::wavpack)
        add_library(wavpack::wavpack INTERFACE IMPORTED)
        set_target_properties(wavpack::wavpack PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${wavpack_INCLUDE_DIRS}")
        set_property(TARGET wavpack::wavpack PROPERTY INTERFACE_LINK_LIBRARIES "${wavpack_LIBRARIES}")
    endif()
endif ()

MARK_AS_ADVANCED(
    WAVPACK_INCLUDE_DIR
    WAVPACK_LIBRARY
)
