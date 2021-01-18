# - Try to find ffmpegthumbnailer
# Once done this will define
#
#  FFMPEGTHUMBNAILER_FOUND - system has FFMpegThumbnailer
#  FFMPEGTHUMBNAILER_INCLUDE_DIRS - the FFMpegThumbnailer include directory
#  FFMPEGTHUMBNAILER_LIBRARIES - Link these to use FFMpegThumbnailer
#

FIND_PACKAGE(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_FFTHUMB QUIET libffmpegthumbnailer)

FIND_PATH(FFMPEGTHUMBNAILER_INCLUDE_DIRS videothumbnailerc.h
    HINTS ${PC_FFTHUMB_INCLUDEDIR} ${PC_FFTHUMB_INCLUDE_DIRS}
    PATH_SUFFIXES ffmpegthumbnailer libffmpegthumbnailer)

find_path(FFMPEGTHUMBNAILER_ROOT_DIR
    NAMES "include/libffmpegthumbnailer/videothumbnailerc.h"
    PATHS ENV FFMPEGTHUMBNAILER_ROOT_DIR)

FIND_LIBRARY(FFMPEGTHUMBNAILER_LIBRARIES
    NAMES libffmpegthumbnailer ffmpegthumbnailer
    HINTS ${PC_FFTHUMB_LIBDIR} ${PC_FFTHUMB_LIBRARY_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFMpegThumbnailer
    REQUIRED_VARS FFMPEGTHUMBNAILER_LIBRARIES FFMPEGTHUMBNAILER_INCLUDE_DIRS)

find_file(_ffmpegthumbnailer_pkg_config_file
    NAMES "libffmpegthumbnailer.pc"
    HINTS ${FFMPEGTHUMBNAILER_ROOT_DIR}
    PATHS ENV FFMPEGTHUMBNAILER_ROOT_DIR
    PATH_SUFFIXES "lib64/pkgconfig" "lib/pkgconfig" "pkgconfig")

if(EXISTS ${_ffmpegthumbnailer_pkg_config_file})
    file(STRINGS "${_ffmpegthumbnailer_pkg_config_file}" _ffmpegthumbnailer_version REGEX "^Version: .*$")
    string(REGEX REPLACE "^.*Version: ([0-9]+).*$" "\\1" FFMPEGTHUMBNAILER_VERSION_MAJOR "${_ffmpegthumbnailer_version}")
    string(REGEX REPLACE "^.*Version: [0-9]+\\.([0-9]+).*$" "\\1" FFMPEGTHUMBNAILER_VERSION_MINOR  "${_ffmpegthumbnailer_version}")
    string(REGEX REPLACE "^.*Version: [0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" FFMPEGTHUMBNAILER_VERSION_PATCH "${_ffmpegthumbnailer_version}")
    set(FFMPEGTHUMBNAILER_VERSION_STRING "${FFMPEGTHUMBNAILER_VERSION_MAJOR}.${FFMPEGTHUMBNAILER_VERSION_MINOR}.${FFMPEGTHUMBNAILER_VERSION_PATCH}")
else(EXISTS ${_ffmpegthumbnailer_pkg_config_file})
    set(FFMPEGTHUMBNAILER_VERSION_STRING "")
endif(EXISTS ${_ffmpegthumbnailer_pkg_config_file})

MARK_AS_ADVANCED(
    FFMPEGTHUMBNAILER_INCLUDE_DIR
    FFMPEGTHUMBNAILER_LIBRARY
)
