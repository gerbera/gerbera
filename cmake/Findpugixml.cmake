# ~~~
# - Try to find libpugixml
# Once done this will define
#
#  PUGIXML_FOUND - system has libpugixml
#  pugixml_INCLUDE_DIRS - the libpugixml include directory
#  pugixml_LIBRARIES - Link these to use PUGIXML
# ~~~

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

pkg_search_module(PC_PUGIXML QUIET libpugixml pugixml)
find_path(
    PUGIXML_INCLUDE_DIR pugixml.hpp
    HINTS ${PC_PUGIXML_INCLUDEDIR} ${PC_PUGIXML_INCLUDE_DIRS})
find_library(
    PUGIXML_LIBRARY pugixml
    HINTS ${PC_PUGIXML_LIBDIR} ${PC_PUGIXML_LIBRARY_DIRS})
set(PUGIXML_VERSION ${PC_PUGIXML_VERSION})

find_package_handle_standard_args(
    pugixml
    REQUIRED_VARS PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR
    VERSION_VAR PUGIXML_VERSION)

if(PUGIXML_FOUND)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin"
       OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD"
       OR CMAKE_SYSTEM_NAME MATCHES ".*(SunOS|Solaris).*")
        set(pugixml_LIBRARIES ${PUGIXML_LIBRARY})
    else()
        set(pugixml_LIBRARIES ${PUGIXML_LIBRARY} ${PC_PUGIXML_LIBRARIES})
    endif()
    set(pugixml_INCLUDE_DIRS ${PUGIXML_INCLUDE_DIR})

    if(NOT TARGET pugixml::pugixml)
        add_library(pugixml::pugixml INTERFACE IMPORTED)
        set_target_properties(pugixml::pugixml PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                          "${pugixml_INCLUDE_DIRS}")
        set_property(TARGET pugixml::pugixml PROPERTY INTERFACE_LINK_LIBRARIES
                                                      "${pugixml_LIBRARIES}")
    endif()
endif()

mark_as_advanced(
    PUGIXML_INCLUDE_DIR PUGIXML_LIBRARY)
