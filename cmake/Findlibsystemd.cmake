# Module defines
#  LIBSYSTEMD_FOUND - libsystemd libraries and includes found
#  LIBSYSTEMD_VERSION - libsystemd version
#  LIBSYSTEMD_INCLUDE_DIRS - the libsystemd include directories
#  LIBSYSTEMD_LIBRARIES - the libsystemd libraries

find_package(PkgConfig QUIET)
pkg_check_modules(_LIBSYSTEMD_PC QUIET "libsystemd")

find_path(LIBSYSTEMD_INCLUDE_DIR systemd/sd-journal.h ${_LIBSYSTEMD_PC_INCLUDE_DIRS} /usr/include /usr/local/include)
find_library(LIBSYSTEMD_LIBRARY NAMES systemd PATHS ${_LIBSYSTEMD_PC_LIBDIR})
set(LIBSYSTEMD_VERSION ${_LIBSYSTEMD_PC_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libsystemd
    REQUIRED_VARS
        LIBSYSTEMD_LIBRARY LIBSYSTEMD_INCLUDE_DIR
    VERSION_VAR
        LIBSYSTEMD_VERSION)

if(libsystemd_FOUND)
  set(LIBSYSTEMD_LIBRARIES ${LIBSYSTEMD_LIBRARY})
  set(LIBSYSTEMD_INCLUDE_DIRS ${LIBSYSTEMD_INCLUDE_DIR})
endif()

MARK_AS_ADVANCED(
    LIBSYSTEMD_INCLUDE_DIR
    LIBSYSTEMD_LIBRARY
)
