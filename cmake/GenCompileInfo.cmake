#*GRB*
#
#  Gerbera - https://gerbera.io/
#
#  GenCompileInfo.cmake - this file is part of Gerbera.
#
#  Copyright (C) 2016-2025 Gerbera Contributors
#
#  Gerbera is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation.
#
#  Gerbera is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
#  $Id$

# Generate the list of compile options
# used to compile Gerbera
#
# Identify the Git branch and revision
# if the directory has .git/ folder

function(generate_compile_info)
    set(COMPILE_INFO_LIST
            "WITH_NPUPNP=${WITH_NPUPNP}"
            "STATIC_LIBUPNP=${STATIC_LIBUPNP}"
            "WITH_MAGIC=${WITH_MAGIC}"
            "WITH_MYSQL=${WITH_MYSQL}"
            "WITH_CURL=${WITH_CURL}"
            "WITH_INOTIFY=${WITH_INOTIFY}"
            "WITH_JS=${WITH_JS}"
            "WITH_TAGLIB=${WITH_TAGLIB}"
            "WITH_AVCODEC=${WITH_AVCODEC}"
            "WITH_FFMPEGTHUMBNAILER=${WITH_FFMPEGTHUMBNAILER}"
            "WITH_EXIF=${WITH_EXIF}"
            "WITH_EXIV2=${WITH_EXIV2}"
            "WITH_MATROSKA=${WITH_MATROSKA}"
            "WITH_WAVPACK=${WITH_WAVPACK}"
            "WITH_SYSTEMD=${WITH_SYSTEMD}"
            "WITH_LASTFM=${WITH_LASTFM}"
            "WITH_DEBUG=${WITH_DEBUG}"
            "WITH_DEBUG_OPTIONS=${WITH_DEBUG_OPTIONS}"
            "WITH_TESTS=${WITH_TESTS}")

    string (REPLACE ";" "\\n" COMPILE_INFO_STR "${COMPILE_INFO_LIST}")
    set(COMPILE_INFO "${COMPILE_INFO_STR}" PARENT_SCOPE)

    # Git Info
    set(GIT_DIR "${CMAKE_SOURCE_DIR}/.git")
    if(EXISTS "${GIT_DIR}")
        # Get the current working branch
        execute_process(
                COMMAND git rev-parse --symbolic-full-name HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Get the tag
        execute_process(
                COMMAND git describe --tags
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Get the latest commit hash
        execute_process(
                COMMAND git rev-parse HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        set(GIT_TAG "")
        set(GIT_BRANCH "")
        set(GIT_COMMIT_HASH "")
    endif()

    set(GIT_BRANCH ${GIT_BRANCH} PARENT_SCOPE)
    set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} PARENT_SCOPE)
    set(GIT_TAG ${GIT_TAG} PARENT_SCOPE)

endfunction()
