# ~~~
# - Try to find libzipppp
# Once done this will define
#
#  ZIPCPPPP_FOUND - system has libzipppp
#  zipppp_INCLUDE_DIRS - the libzipppp include directory
#  zipppp_LIBRARIES - Link these to use libzipppp
# ~~~

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

pkg_search_module(PC_LIBZIPPP QUIET libzippp zippp)
find_path(
    ZIPPP_INCLUDE_DIR libzippp/libzippp.h
    HINTS ${PC_LIBZIPPP_INCLUDEDIR} ${PC_LIBZIPPP_INCLUDE_DIRS})
find_library(
    ZIPPP_LIBRARY
    NAMES zippp libzippp
    HINTS ${PC_LIBZIPPP_LIBDIR} ${PC_LIBZIPPP_LIBRARY_DIRS})

set(ZIPPP_VERSION ${PC_LIBZIPPP_VERSION})

find_package_handle_standard_args(
    libzippp
    REQUIRED_VARS ZIPPP_LIBRARY ZIPPP_INCLUDE_DIR
    VERSION_VAR ZIPPP_VERSION)

if(LIBZIPPP_FOUND)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin"
       OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD"
       OR CMAKE_SYSTEM_NAME MATCHES ".*(SunOS|Solaris).*")
        set(ZIPPP_LIBRARIES ${ZIPPP_LIBRARY})
    else()
        set(ZIPPP_LIBRARIES ${ZIPPP_LIBRARY} ${PC_ZIPPP_LIBRARIES})
    endif()
    set(ZIPPP_INCLUDE_DIRS ${ZIPPP_INCLUDE_DIR})

    if(NOT TARGET zippp::zippp)
        add_library(zippp::zippp INTERFACE IMPORTED)
        set_target_properties(zippp::zippp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                      "${ZIPPP_INCLUDE_DIRS}")
        set_property(TARGET zippp::zippp PROPERTY INTERFACE_LINK_LIBRARIES "${ZIPPP_LIBRARIES}")
    endif()
else()
    set(ZIPPP_LIBRARIES ${PC_LIBZIPPP_LIBRARIES})
endif()

mark_as_advanced(
    ZIPPP_INCLUDE_DIR ZIPPP_LIBRARY)
