# - Try to find LibNPUPnP (npupnp) 4.1.2
# Once done this will define
#  NPUPNP_FOUND - System has LibNPUPnP
#  UPNP_INCLUDE_DIRS - The LibNPUPnP include directories
#  NPUPNP_LIBRARIES - The libraries needed to use LibNPUPnP
#  NPUPNP_VERSION_STRING - The version of LibNPUPnP found
#  NPUPNP_HAS_IPV6 - If LibNPUPnP was built with IPv6 support
#  NPUPNP_HAS_REUSEADDR - If LibNPUPnP was built with SO_REUSEADDR support
find_package(PkgConfig QUIET)
pkg_search_module (PC_NPUPNP libnpupnp QUIET)

find_path(NPUPNP_INCLUDE_DIR upnp.h
    HINTS ${PC_NPUPNP_INCLUDEDIR} ${PC_NPUPNP_INCLUDEDIR}/npupnp ${PC_NPUPNP_INCLUDE_DIRS})
message(STATUS "found npupnp include at ${NPUPNP_INCLUDE_DIR}")

if (STATIC_LIBUPNP)
    set(OLD_SUFFIX ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

find_library(NPUPNP_LIBRARY
    NAMES libnpupnp npupnp
    HINTS ${PC_NPUPNP_LIBDIR} ${PC_NPUPNP_LIBRARY_DIRS})

# Restore
if (STATIC_LIBUPNP)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${OLD_SUFFIX})
endif()

if(EXISTS ${NPUPNP_INCLUDE_DIR}/upnpconfig.h)
    file (STRINGS ${NPUPNP_INCLUDE_DIR}/upnpconfig.h npupnp_ver_str REGEX "^#define[ \t]+NPUPNP_VERSION_STRING[ \t]+\".+\"")
    string(REGEX REPLACE "^#define[ \t]+NPUPNP_VERSION_STRING[ \t]+\"([^\"]+)\".*" "\\1" NPUPNP_VERSION "${npupnp_ver_str}")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NPUPNP
    REQUIRED_VARS NPUPNP_LIBRARY NPUPNP_INCLUDE_DIR
    VERSION_VAR NPUPNP_VERSION)

if (NPUPNP_FOUND)
    if(NOT TARGET NPUPNP::NPUPNP)
        add_library(NPUPNP::NPUPNP SHARED IMPORTED)
        set_target_properties(NPUPNP::NPUPNP PROPERTIES
                IMPORTED_LOCATION ${NPUPNP_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES ${NPUPNP_INCLUDE_DIR}
                VERSION ${NPUPNP_VERSION}
        )
    endif()
endif ()

MARK_AS_ADVANCED(
    NPUPNP_INCLUDE_DIR
    NPUPNP_LIBRARY
)
