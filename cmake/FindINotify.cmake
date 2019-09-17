# This module defines
#  INOTIFY_INCLUDE_DIR, where to find inotify.h, etc.
#  INOTIFY_FOUND, If false, do not try to use inotify.
#  INOTIFY_LIBRARY, where to find the libinotify library (BSD).

message(STATUS "Looking for native inotify")

find_path(INOTIFY_INCLUDE_DIR sys/inotify.h
    HINTS /usr/include/${CMAKE_LIBRARY_ARCHITECTURE})

if (INOTIFY_INCLUDE_DIR AND (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "SunOS"))
    message(STATUS "Looking for native inotify - found")
    SET(INOTIFY_FOUND TRUE)
    SET(INOTIFY_LIBRARY "")
else()
    message(STATUS "Looking for native inotify - not found")
    message(STATUS "Looking for libinotify")
    find_library(INOTIFY_LIBRARY inotify)
    if (INOTIFY_LIBRARY)
        message(STATUS "Looking for libinotify - found")
    endif()
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(INOTIFY DEFAULT_MSG INOTIFY_LIBRARY INOTIFY_INCLUDE_DIR)
endif()

mark_as_advanced(INOTIFY_LIBRARY INOTIFY_INCLUDE_DIR)
