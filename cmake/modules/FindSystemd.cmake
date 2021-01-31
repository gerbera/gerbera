#
# Find systemd using PkgConfig
#
# Defines:
# SYSTEMD_FOUND: if systemd was found
# SYSTEMD_UNIT_DIR: path to install system unit files

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
pkg_check_modules(SYSTEMD systemd QUIET)

if (SYSTEMD_FOUND)
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE}
        --variable=systemdsystemunitdir systemd
        OUTPUT_VARIABLE SYSTEMD_SERVICES_INSTALL_DIR)
    string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_UNIT_DIR "${SYSTEMD_SERVICES_INSTALL_DIR}")
endif()

find_package_handle_standard_args(Systemd DEFAULT_MSG SYSTEMD_FOUND)
