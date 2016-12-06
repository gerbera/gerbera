find_package(PkgConfig)
pkg_check_modules (PC_UPNP REQUIRED libupnp)

find_path(UPNP_INCLUDE_DIR upnp/upnp.h
    HINTS
    ${PC_UPNP_INCLUDEDIR}
    ${PC_UPNP_INCLUDE_DIRS}
    PATH_SUFFIXES upnp)
find_library(UPNP_UPNP_LIBRARY
    NAMES libupnp upnp upnp4
    HINTS
    ${PC_UPNP_LIBDIR}
    ${PC_UPNP_LIBRARY_DIRS})
find_library(UPNP_IXML_LIBRARY
    NAMES libixml ixml ixml4
    HINTS
    ${PC_UPNP_LIBDIR}
    ${PC_UPNP_LIBRARY_DIRS})
find_library(UPNP_THREADUTIL_LIBRARY
    NAMES libthreadutil threadutil threadutil4
    HINTS
    ${PC_UPNP_LIBDIR}
    ${PC_UPNP_LIBRARY_DIRS})

if (UPNP_INCLUDE_DIR AND UPNP_UPNP_LIBRARY)
    set (UPNP_FOUND TRUE)
endif (UPNP_INCLUDE_DIR AND UPNP_UPNP_LIBRARY)

if (UPNP_FOUND)
    set (UPNP_LIBRARIES ${UPNP_UPNP_LIBRARY} ${UPNP_IXML_LIBRARY} ${UPNP_THREADUTIL_LIBRARY} )

    if(UPNP_INCLUDE_DIR AND EXISTS "${UPNP_INCLUDE_DIR}/upnp/upnpconfig.h")
        file (STRINGS ${UPNP_INCLUDE_DIR}/upnp/upnpconfig.h _UPNP_DEFS REGEX "^[ \t]*#define[ \t]+UPNP_VERSION_(MAJOR|MINOR|PATCH)")
        string (REGEX REPLACE ".*UPNP_VERSION_MAJOR ([0-9]+).*" "\\1" UPNP_MAJOR_VERSION "${_UPNP_DEFS}")
        string (REGEX REPLACE ".*UPNP_VERSION_MINOR ([0-9]+).*" "\\1" UPNP_MINOR_VERSION "${_UPNP_DEFS}")
        string (REGEX REPLACE ".*UPNP_VERSION_PATCH ([0-9]+).*" "\\1" UPNP_PATCH_VERSION "${_UPNP_DEFS}")
        set (UPNP_VERSION_STRING "${UPNP_MAJOR_VERSION}.${UPNP_MINOR_VERSION}.${UPNP_PATCH_VERSION}")
        message (STATUS "Found libupnp version: ${UPNP_VERSION_STRING}")
    endif()

    if(UPNP_INCLUDE_DIR AND EXISTS "${UPNP_INCLUDE_DIR}/upnp/TemplateInclude.h")
        message (WARNING "\n!! You are using a very old 1.8 snapshot. Please upgrade to a newer snapshot from upstream (https://github.com/mrjimenez/pupnp) !!\n")
        message (STATUS "libupnp: Enabling old snapshot compat.")
        set (UPNP_OLD_SNAPSHOT 1)
    endif()

    message (STATUS "Found the libupnp libraries: ${UPNP_LIBRARIES}")
    message (STATUS "Found the libupnp headers: ${UPNP_INCLUDE_DIR}")
else (UPNP_FOUND)
    message (STATUS "Could not find libupnp")
endif (UPNP_FOUND)

MARK_AS_ADVANCED(
    UPNP_INCLUDE_DIR
    UPNP_IXML_LIBRARY
    UPNP_UPNP_LIBRARY
    UPNP_THREADUTIL_LIBRARY
)
