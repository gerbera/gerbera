# - Find libexiv2
# Find the native EXIV2 includes and library.
# Once done this will define:
#
#  EXIV2_ROOT_DIR       - The base directory of EXIV2 library.
#  EXIV2_INCLUDE_DIRS   - Where to find exiv2-*.h header files.
#  EXIV2_LIBRARIES      - Libraries to be linked with executable.
#  EXIV2_FOUND          - True if EXIV2 library is found.
#
#  EXIV2_VERSION_STRING - The version of EXIV2 library found (x.y.z)
#  EXIV2_VERSION_MAJOR  - The major version of EXIV2 library
#  EXIV2_VERSION_MINOR  - The minor version of EXIV2 library
#  EXIV2_VERSION_PATCH  - The patch version of EXIV2 library
#
# Optional input variables:
#
#  EXIV2_USE_STATIC_LIB - This can be set to ON to use the static version of
#      the library. If it does not exist in the system, a warning is printed
#      and the shared library is used.
#
# If exiv2 is installed in a custom folder (this usually happens on Windows)
# the environment variable EXIV2_ROOT_DIR can be set to the base folder of
# the library.
#
#=============================================================================
# Copyright (C) 2011 Simone Rossetto <simros85@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#=============================================================================

### FIND section ###
find_path(EXIV2_ROOT_DIR
    NAMES "include/exiv2/exv_conf.h"
    PATHS ENV EXIV2_ROOT_DIR)

find_path(EXIV2_INCLUDE_DIR
    NAMES "exiv2/exv_conf.h"
    HINTS ${EXIV2_ROOT_DIR}
    PATHS ENV EXIV2_ROOT_DIR
    PATH_SUFFIXES "include")

find_library(EXIV2_LIBRARY
    NAMES "exiv2"
    HINTS ${EXIV2_ROOT_DIR}
    PATHS ENV EXIV2_ROOT_DIR
    PATH_SUFFIXES "lib")

find_file(_exiv2_pkg_config_file
    NAMES "exiv2.pc"
    HINTS ${EXIV2_ROOT_DIR}
    PATHS ENV EXIV2_ROOT_DIR
    PATH_SUFFIXES "lib/pkgconfig" "pkgconfig")

if(EXISTS ${_exiv2_pkg_config_file})
    file(STRINGS "${_exiv2_pkg_config_file}" _exiv2_version REGEX "^Version: .*$")
    string(REGEX REPLACE "^.*Version: ([0-9]+).*$" "\\1" EXIV2_VERSION_MAJOR "${_exiv2_version}")
    string(REGEX REPLACE "^.*Version: [0-9]+\\.([0-9]+).*$" "\\1" EXIV2_VERSION_MINOR  "${_exiv2_version}")
    string(REGEX REPLACE "^.*Version: [0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" EXIV2_VERSION_PATCH "${_exiv2_version}")
    set(EXIV2_VERSION_STRING "${EXIV2_VERSION_MAJOR}.${EXIV2_VERSION_MINOR}.${EXIV2_VERSION_PATCH}")
else(EXISTS ${_exiv2_pkg_config_file})
    set(EXIV2_VERSION_STRING "")
endif(EXISTS ${_exiv2_pkg_config_file})


### OPTIONS and COMPONENTS section ###
if(EXIV2_USE_STATIC_LIB)
    set(_exiv2_shared_lib ${EXIV2_LIBRARY})
    if(WIN32 AND NOT MSVC)
        string(REPLACE ".dll.a" ".a" _exiv2_static_lib ${_exiv2_shared_lib})
    elseif(APPLE)
        string(REPLACE ".dylib" ".a" _exiv2_static_lib ${_exiv2_shared_lib})
    elseif(UNIX)
        string(REPLACE ".so" ".a" _exiv2_static_lib ${_exiv2_shared_lib})
    endif()

    if(EXISTS ${_exiv2_static_lib})
        set(EXIV2_LIBRARY ${_exiv2_static_lib})
    else(EXISTS ${_exiv2_static_lib})
        message(WARNING "EXIV2 static library does not exist, using the shared one.")
        set(EXIV2_LIBRARY ${_exiv2_shared_lib})
    endif(EXISTS ${_exiv2_static_lib})
endif(EXIV2_USE_STATIC_LIB)

mark_as_advanced(
    EXIV2_LIBRARY
    EXIV2_INCLUDE_DIR
    _exiv2_pkg_config_file
    _exiv2_shared_lib
    _exiv2_static_lib)


### final check ###
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EXIV2
    REQUIRED_VARS EXIV2_LIBRARY EXIV2_INCLUDE_DIR
    VERSION_VAR EXIV2_VERSION_STRING)

if(EXIV2_FOUND)
    set(EXIV2_INCLUDE_DIRS ${EXIV2_INCLUDE_DIR} "${EXIV2_INCLUDE_DIR}/exiv2")
    set(EXIV2_LIBRARIES ${EXIV2_LIBRARY})
endif(EXIV2_FOUND)
