# - Try to find libffmpeg
# Once done this will define
#
#  FFMPEG_FOUND - system has libffmpeg
#  FFMPEG_INCLUDE_DIRS - the libffmpeg include directory
#  FFMPEG_LIBRARIES - Link these to use FFMPEG

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

# FFMPEG
pkg_search_module(PC_FFMPEG QUIET libavcodec IMPORTED_TARGET)
find_path(FFMPEG_INCLUDE_DIR version.h
        HINTS ${PC_FFMPEG_INCLUDEDIR}
        PATH_SUFFIXES ffmpeg)
find_library(FFMPEG_LIBRARY ffmpeg
        HINTS ${PC_FFMPEG_LIBDIR})

find_package_handle_standard_args(FFMPEG DEFAULT_MSG FFMPEG_LIBRARY FFMPEG_INCLUDE_DIR)

if (FFMPEG_FOUND)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARY})
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})

    add_library(ffmpeg::ffmpeg ALIAS PkgConfig::PC_FFMPEG)
endif()

mark_as_advanced(
    FFMPEG_INCLUDE_DIR
    FFMPEG_LIBRARY
)