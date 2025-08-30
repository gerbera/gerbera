# - Try to find libjsoncpp
# Once done this will define
#
#  JSONCPP_FOUND - system has libjsoncpp
#  jsoncpp_INCLUDE_DIRS - the jsoncpp include directory
#  jsoncpp_LIBRARIES - Link these to use JSONCPP
#

INCLUDE(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

pkg_check_modules(PC_JSONCPP QUIET libjsoncpp)
if (JSONCPP_FOUND)
    find_path(JSONCPP_INCLUDE_DIR json/json.h
        HINTS ${PC_JSONCPP_INCLUDEDIR} ${PC_JSONCPP_INCLUDE_DIRS})
    FIND_LIBRARY(JSONCPP_LIBRARY
        NAMES jsoncpp libjsoncpp
        HINTS ${PC_JSONCPP_LIBDIR} ${PC_JSONCPP_LIBRARY_DIRS})
else ()
    pkg_check_modules(PC_JSONCPP QUIET jsoncpp)
    find_path(JSONCPP_INCLUDE_DIR json/json.h
        HINTS ${PC_JSONCPP_INCLUDEDIR} ${PC_JSONCPP_INCLUDE_DIRS})
    FIND_LIBRARY(JSONCPP_LIBRARY
        NAMES jsoncpp libjsoncpp
        HINTS ${PC_JSONCPP_LIBDIR} ${PC_JSONCPP_LIBRARY_DIRS})
endif ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(jsoncpp
    REQUIRED_VARS JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR)

if (JSONCPP_FOUND)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
       set (jsoncpp_LIBRARIES ${JSONCPP_LIBRARY})
    else ()
       set (jsoncpp_LIBRARIES ${JSONCPP_LIBRARY} ${PC_JSONCPP_LIBRARIES})
    endif ()
    set (jsoncpp_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})

    if(NOT TARGET jsoncpp::jsoncpp)
        add_library(jsoncpp::jsoncpp INTERFACE IMPORTED)
        set_target_properties(jsoncpp::jsoncpp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${jsoncpp_INCLUDE_DIRS}")
        set_property(TARGET jsoncpp::jsoncpp PROPERTY INTERFACE_LINK_LIBRARIES "${jsoncpp_LIBRARIES}")
    endif()
endif ()

MARK_AS_ADVANCED(
    JSONCPP_INCLUDE_DIR
    JSONCPP_LIBRARY
)
