#
# Find systemd using PkgConfig
#
#

include(FindPkgConfig)
pkg_check_modules(SYSTEMD systemd QUIET)

if(SYSTEMD_FOUND)
    message(STATUS "Found systemd: ${PKG_CONFIG_EXECUTABLE}")
endif()