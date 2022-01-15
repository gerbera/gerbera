# - Try to find LibUPnP (pupnp) 1.12
# Once done this will define
#  UPNP_FOUND - System has LibUPnP
#  UPNP_INCLUDE_DIRS - The LibUPnP include directories
#  UPNP_LIBRARIES - The libraries needed to use LibUPnP
#  UPNP_VERSION_STRING - The version of LinUPnP found
#  UPNP_HAS_IPV6 - If LinUPnP was built with IPv6 support
#  UPNP_HAS_REUSEADDR - If LinUPnP was built with SO_REUSEADDR support
find_package(PkgConfig QUIET)
pkg_search_module (PC_UPNP QUIET libupnp)

find_path(UPNP_INCLUDE_DIR upnp.h
    HINTS ${PC_UPNP_INCLUDEDIR} ${PC_UPNP_INCLUDE_DIRS}
    PATH_SUFFIXES upnp)

if (STATIC_LIBUPNP)
    set(OLD_SUFFIX ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

find_library(UPNP_UPNP_LIBRARY
    NAMES libupnp upnp
    HINTS ${PC_UPNP_LIBDIR} ${PC_UPNP_LIBRARY_DIRS})

find_library(UPNP_IXML_LIBRARY
    NAMES libixml ixml
    HINTS ${PC_UPNP_LIBDIR} ${PC_UPNP_LIBRARY_DIRS})

# Restore
if (STATIC_LIBUPNP)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${OLD_SUFFIX})
endif()

if(EXISTS ${UPNP_INCLUDE_DIR}/upnpconfig.h)
    file (STRINGS ${UPNP_INCLUDE_DIR}/upnpconfig.h upnp_ver_str REGEX "^#define[ \t]+UPNP_VERSION_STRING[ \t]+\".+\"")
    string(REGEX REPLACE "^#define[ \t]+UPNP_VERSION_STRING[ \t]+\"([^\"]+)\".*" "\\1" UPNP_VERSION "${upnp_ver_str}")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UPNP
    REQUIRED_VARS UPNP_UPNP_LIBRARY UPNP_INCLUDE_DIR
    VERSION_VAR UPNP_VERSION)

if (UPNP_FOUND)
    if(NOT TARGET UPNP::UPNP)
        add_library(UPNP::UPNP SHARED IMPORTED)
        set_target_properties(UPNP::UPNP PROPERTIES
                IMPORTED_LOCATION ${UPNP_UPNP_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES ${UPNP_INCLUDE_DIR}
                INTERFACE_LINK_LIBRARIES ${UPNP_IXML_LIBRARY}
                VERSION ${UPNP_VERSION}
        )

    endif()
endif ()

MARK_AS_ADVANCED(
    UPNP_INCLUDE_DIR
    UPNP_IXML_LIBRARY
    UPNP_UPNP_LIBRARY
)
