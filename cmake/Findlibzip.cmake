# - Try to find libzipp
# Once done this will define
#
#  ZIPCPPPP_FOUND - system has libzipp
#  zipp_INCLUDE_DIRS - the libzipp include directory
#  zipp_LIBRARIES - Link these to use libzipp
#

INCLUDE(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

pkg_search_module(PC_LIBZIP QUIET libzip zip)
find_path(ZIP_INCLUDE_DIR zip.h
    HINTS ${PC_LIBZIP_INCLUDEDIR} ${PC_LIBZIP_INCLUDE_DIRS})
FIND_LIBRARY(ZIP_LIBRARY
    NAMES zip libzip
    HINTS ${PC_LIBZIP_LIBDIR} ${PC_LIBZIP_LIBRARY_DIRS})

set(ZIP_VERSION ${PC_LIBZIP_VERSION})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(libzip
    REQUIRED_VARS
        ZIP_LIBRARY ZIP_INCLUDE_DIR
    VERSION_VAR
        ZIP_VERSION)

if (LIBZIP_FOUND)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES ".*(SunOS|Solaris).*")
       set (ZIP_LIBRARIES ${ZIP_LIBRARY})
    else ()
       set (ZIP_LIBRARIES ${ZIP_LIBRARY} ${PC_ZIP_LIBRARIES})
    endif ()
    set (ZIP_INCLUDE_DIRS ${ZIP_INCLUDE_DIR})

    if(NOT TARGET zip::zip)
        add_library(zip::zip INTERFACE IMPORTED)
        set_target_properties(zip::zip PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ZIP_INCLUDE_DIRS}")
        set_property(TARGET zip::zip PROPERTY INTERFACE_LINK_LIBRARIES "${ZIP_LIBRARIES}")
    endif()
else ()
   set (ZIP_LIBRARIES ${PC_LIBZIP_LIBRARIES})
endif ()

MARK_AS_ADVANCED(
    ZIP_INCLUDE_DIR
    ZIP_LIBRARY
)
