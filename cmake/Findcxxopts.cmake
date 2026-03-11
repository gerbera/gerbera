# - Try to find cxxopts
# Once done this will define
#  CXXOPTS_FOUND - System has cxxopts
#  CXXOPTS_INCLUDE_DIRS - The cxxopts include directories

find_package(PkgConfig QUIET)

pkg_search_module(PC_CXXOPTS QUIET cxxopts)
find_path(CXXOPTS_INCLUDE_DIR cxxopts.hpp
    HINTS ${PC_CXXOPTS_INCLUDEDIR} ${PC_CXXOPTS_INCLUDE_DIRS})

set(CXXOPTS_VERSION ${PC_CXXOPTS_VERSION})

include(FindPackageHandleStandardArgs)
if (CXXOPTS_INCLUDE_DIR)
    set(CXXOPTS_INCLUDE_DIRS ${CXXOPTS_INCLUDE_DIR})
else()
    find_path(CXXOPTS_INCLUDE_DIR cxxopts.hpp
              PATHS "src/contrib" )
    set(CXXOPTS_INCLUDE_DIRS ${CXXOPTS_INCLUDE_DIR})
    set(CXXOPTS_VERSION "3.3.1")
    message("-- Using bundled cxxopts")
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(cxxopts
    REQUIRED_VARS
        CXXOPTS_INCLUDE_DIR
    VERSION_VAR
        CXXOPTS_VERSION)

add_library(cxxopt::cxxopt INTERFACE IMPORTED)
set_target_properties(cxxopt::cxxopt PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CXXOPTS_INCLUDE_DIRS}")

mark_as_advanced(CXXOPTS_INCLUDE_DIR)
