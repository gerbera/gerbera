find_package(PkgConfig)
pkg_check_modules (PC_UPNP REQUIRED libupnp)

set (UPNP_DEFINITIONS ${PC_UPNP_CFLAGS_OTHER})

find_path(UPNP_INCLUDE_DIR upnp/upnp.h
    HINTS
    ${PC_UPNP_INCLUDEDIR}
    ${PC_UPNP_INCLUDE_DIRS}
    PATH_SUFFIXES upnp)
find_library(UPNP_UPNP_LIBRARY upnp
    HINTS
    ${PC_UPNP_LIBDIR}
    ${PC_UPNP_LIBRARY_DIRS})
find_library(UPNP_IXML_LIBRARY ixml
    HINTS
    ${PC_UPNP_LIBDIR}
    ${PC_UPNP_LIBRARY_DIRS})
find_library(UPNP_THREADUTIL_LIBRARY threadutil
    HINTS
    ${PC_UPNP_LIBDIR}
    ${PC_UPNP_LIBRARY_DIRS})

if (UPNP_INCLUDE_DIR AND UPNP_UPNP_LIBRARY)
    set (UPNP_FOUND TRUE)
endif (UPNP_INCLUDE_DIR AND UPNP_UPNP_LIBRARY)

if (UPNP_FOUND)
    set (UPNP_LIBRARIES ${UPNP_UPNP_LIBRARY} ${UPNP_IXML_LIBRARY} ${UPNP_THREADUTIL_LIBRARY} )
    if (APPLE)
        set (UPNP_LIBRARIES ${UPNP_LIBRARY} ${PC_UPNP_LDFLAGS})
    endif (APPLE)
    message (STATUS "Found the upnp libraries at ${UPNP_LIBRARY}")
    message (STATUS "Found the upnp headers at ${UPNP_INCLUDE_DIR}")
else (UPNP_FOUND)
    message (STATUS "Could not find upnp")
endif (UPNP_FOUND)

MARK_AS_ADVANCED(
    UPNP_INCLUDE_DIR
    UPNP_LIBRARY
)
