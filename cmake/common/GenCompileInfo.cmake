#
# Generate the list of compile options
# used to compile Gerbera
#
# Identify the Git branch and revision
# if the directory has .git/ folder
#
function(generate_compile_info)
    set(COMPILE_INFO_LIST
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
            "WITH_SYSTEMD=${WITH_SYSTEMD}"
            "WITH_LASTFM=${WITH_LASTFM}"
            "WITH_DEBUG=${WITH_DEBUG}"
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
                COMMAND git describe --tags --dirty=+d
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
        set(GIT_BRANCH "")
        set(GIT_COMMIT_HASH "")
    endif()

    set(GIT_BRANCH ${GIT_BRANCH} PARENT_SCOPE)
    set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} PARENT_SCOPE)
    set(GIT_TAG ${GIT_TAG} PARENT_SCOPE)

endfunction()