find_package(fmt REQUIRED)
set(GERBERA_FMT_LIB fmt::fmt)

find_package(spdlog)
if(spdlog_FOUND)
    set(GERBERA_SPDLOG_LIB spdlog::spdlog)

    if(NOT SPDLOG_FMT_EXTERNAL)
        message(WARNING [=[
spdlog is built without SPDLOG_FMT_EXTERNAL.
Since Gerbera uses fmt library internally the spdlog's bundled version wlll be in conflict
It is strongly recommended to rebuild spdlog without bundled fmt]=])
        if(spdlog_VERSION VERSION_GREATER_EQUAL "1.4.0" AND fmt_VERSION VERSION_LESS "6.0.0")
            message(FATAL_ERROR [=[
The version of spdlog >= 1.4 bundles fmt version 6, but an older version of fmt was found.
The current combination won't link.
Please upgrade fmt or build spdlog with SPDLOG_FMT_EXTERNAL=ON]=])
        endif()
    endif()
else()
    # See if it's just installed as a library, as not all installs have .cmake files
    message(STATUS "Looking for spdlog library")
    find_library(SPDLOG_LIBRARY spdlog)
    if(SPDLOG_LIBRARY)
        message(STATUS "Found spdlog: ${SPDLOG_LIBRARY}")
        set(GERBERA_SPDLOG_LIB ${SPDLOG_LIBRARY})
    else()
        message(FATAL_ERROR "spdlog library is required")
    endif()
endif()

find_package (Pugixml REQUIRED)
set(GERBERA_PUGIXML_LIB ${PUGIXML_LIBRARIES})
include_directories(${PUGIXML_INCLUDE_DIRS})

find_package(UUID REQUIRED)
include_directories(${UUID_INCLUDE_DIRS})
set(GERBERA_UUID_LIB ${UUID_LIBRARIES})
if (FOUND_BSD_UUID)
    add_definitions(-DBSD_NATIVE_UUID)
endif()

find_package(Iconv REQUIRED)
include_directories(${ICONV_INCLUDE_DIR})
set(GERBERA_ICONV_LIB ${ICONV_LIBRARIES})
if (ICONV_SECOND_ARGUMENT_IS_CONST)
    add_definitions(-DICONV_CONST)
endif()

find_package (SQLite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})
set(GERBERA_SQLITE3_LIB ${SQLITE3_LIBRARIES})
add_definitions(-DHAVE_SQLITE3)

find_package (ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set (GERBERA_ZLIB_LIB ${ZLIB_LIBRARIES})

if(WITH_TESTS)
    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)
endif()
