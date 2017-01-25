# - Try to find LibUPnP (pupnp) 1.8
# Once done this will define
#  UPNP_FOUND - System has LibUPnP
#  UPNP_INCLUDE_DIRS - The LibUPnP include directories
#  UPNP_LIBRARIES - The libraries needed to use LibUPnP
#  UPNP_VERSION_STRING - The version of LinUPnP found
#  UPNP_HAS_IPV6 - If LinUPnP was built with IPv6 support

find_package(PkgConfig QUIET)
pkg_check_modules (PC_UPNP QUIET libupnp-1.8)

find_path(UPNP_INCLUDE_DIR upnp-1.8/upnp.h
    HINTS ${PC_UPNP_INCLUDEDIR} ${PC_UPNP_INCLUDE_DIRS}
    PATH_SUFFIXES upnp)
find_library(UPNP_UPNP_LIBRARY
    NAMES libupnp-1.8 upnp-1.8 upnp4
    HINTS ${PC_UPNP_LIBDIR} ${PC_UPNP_LIBRARY_DIRS})
find_library(UPNP_IXML_LIBRARY
    NAMES libixml-1.8 ixml-1.8 ixml4
    HINTS ${PC_UPNP_LIBDIR} ${PC_UPNP_LIBRARY_DIRS})
find_library(UPNP_THREADUTIL_LIBRARY
    NAMES libthreadutil-1.8 threadutil-1.8 threadutil4
    HINTS ${PC_UPNP_LIBDIR} ${PC_UPNP_LIBRARY_DIRS})

if(EXISTS "${UPNP_INCLUDE_DIR}/upnp-1.8/upnpconfig.h")
    file (STRINGS ${UPNP_INCLUDE_DIR}/upnp-1.8/upnpconfig.h _UPNP_DEFS REGEX "^[ \t]*#define[ \t]+UPNP_VERSION_(MAJOR|MINOR|PATCH)")
    string (REGEX REPLACE ".*UPNP_VERSION_MAJOR ([0-9]+).*" "\\1" UPNP_MAJOR_VERSION "${_UPNP_DEFS}")
    string (REGEX REPLACE ".*UPNP_VERSION_MINOR ([0-9]+).*" "\\1" UPNP_MINOR_VERSION "${_UPNP_DEFS}")
    string (REGEX REPLACE ".*UPNP_VERSION_PATCH ([0-9]+).*" "\\1" UPNP_PATCH_VERSION "${_UPNP_DEFS}")
    set (UPNP_VERSION_STRING "${UPNP_MAJOR_VERSION}.${UPNP_MINOR_VERSION}.${UPNP_PATCH_VERSION}")

    file (STRINGS ${UPNP_INCLUDE_DIR}/upnp-1.8/upnpconfig.h _UPNP_V6 REGEX "^[ \t]*#define[ \t]+UPNP_ENABLE_IPV6[ \t]+(1)$")
    MESSAGE(${_UPNP_V6})
    if(_UPNP_V6)
        set (UPNP_HAS_IPV6 1)
    else()
        set (UPNP_HAS_IPV6 0)
    endif()

endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UPnP
    REQUIRED_VARS UPNP_UPNP_LIBRARY UPNP_INCLUDE_DIR
    VERSION_VAR UPNP_VERSION_STRING)

if (UPNP_FOUND)
    set (UPNP_LIBRARIES ${UPNP_UPNP_LIBRARY} ${UPNP_IXML_LIBRARY} ${UPNP_THREADUTIL_LIBRARY} )
    set (UPNP_INCLUDE_DIRS ${UPNP_INCLUDE_DIR} )
endif ()

MARK_AS_ADVANCED(
    UPNP_INCLUDE_DIR
    UPNP_IXML_LIBRARY
    UPNP_UPNP_LIBRARY
    UPNP_THREADUTIL_LIBRARY
)
