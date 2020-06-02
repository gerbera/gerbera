# TODO: If we can raise required version to 3.11 there will be no need for iconv or 3.14 for sqlite
cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(GERBERA_MAJOR_VERSION 1)
set(GERBERA_MINOR_VERSION 6)
set(GERBERA_PATCH_VERSION 0)
set(GERBERA_RELEASE "_git")
set(GERBERA_VERSION "${GERBERA_MAJOR_VERSION}.${GERBERA_MINOR_VERSION}.${GERBERA_PATCH_VERSION}${GERBERA_RELEASE}")

project(Gerbera
        VERSION "${GERBERA_MAJOR_VERSION}.${GERBERA_MINOR_VERSION}.${GERBERA_PATCH_VERSION}")

# Leverage a modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL)

if (CONAN_EXPORTED)
    conan_message(STATUS "Conan: called by CMake conan helper")
    # Propagate variables set by the helper
    # Normally conan_basic_setup should be called, but we rely
    # on cmake_find_package generator.
    # Do only some checks and setup here.
    conan_set_std()
    conan_set_fpic()
    conan_check_compiler()
    conan_set_libcxx()
    conan_set_vs_runtime()
endif()

set(CMAKE_VERBOSE_MAKEFILE off CACHE BOOL "Show verbose build commands")
set(WITH_MAGIC          1 CACHE BOOL "Use libmagic to identify file mime types")
set(WITH_MYSQL          0 CACHE BOOL "Store media information in MySQL DB")
set(WITH_CURL           1 CACHE BOOL "CURL required for online services")
set(WITH_INOTIFY        1 CACHE BOOL "Enable Inotify file monitoring support")
set(WITH_JS             1 CACHE BOOL "Enable JavaScript for custom import script")
set(WITH_TAGLIB         1 CACHE BOOL "Use TagLib to extract audio file metadata")
set(WITH_AVCODEC        0 CACHE BOOL "Enable ffmpeg/libav")
set(WITH_FFMPEGTHUMBNAILER 0 CACHE BOOL "Enable Thumbnail generation")
set(WITH_EXIF           1 CACHE BOOL "Use libexif to extract image metadata")
set(WITH_EXIV2          0 CACHE BOOL "Use libexiv2 to extract image metadata")
set(WITH_MATROSKA       1 CACHE BOOL "Use libmatroska to extract video/mkv metadata")
set(WITH_SYSTEMD        1 CACHE BOOL "Install Systemd unit file")
set(WITH_LASTFM         0 CACHE BOOL "Enable LastFM")
set(WITH_DEBUG          1 CACHE BOOL "Enables debug logging")
set(WITH_TESTS          0 CACHE BOOL "Enables Unit Tests")

set(libgerberaFILES
        src/action_request.cc
        src/action_request.h
        src/autoscan.cc
        src/autoscan.h
        src/autoscan_inotify.cc
        src/autoscan_inotify.h
        src/cds_objects.cc
        src/cds_objects.h
        src/cds_resource.cc
        src/cds_resource.h
        src/common.h
        src/config/client_config.h
        src/config/client_config.cc
        src/config/config_generator.h
        src/config/config_generator.cc
        src/config/config_manager.cc
        src/config/config_manager.h
        src/config/config_options.h
        src/content_manager.cc
        src/content_manager.h
        src/contrib/md5.c
        src/contrib/md5.h
        src/device_description_handler.cc
        src/device_description_handler.h
        src/exceptions.cc
        src/exceptions.h
        src/file_request_handler.cc
        src/file_request_handler.h
        src/iohandler/buffered_io_handler.cc
        src/iohandler/buffered_io_handler.h
        src/iohandler/curl_io_handler.cc
        src/iohandler/curl_io_handler.h
        src/iohandler/file_io_handler.cc
        src/iohandler/file_io_handler.h
        src/iohandler/io_handler_buffer_helper.cc
        src/iohandler/io_handler_buffer_helper.h
        src/iohandler/io_handler.cc
        src/iohandler/io_handler_chainer.cc
        src/iohandler/io_handler_chainer.h
        src/iohandler/io_handler.h
        src/iohandler/mem_io_handler.cc
        src/iohandler/mem_io_handler.h
        src/iohandler/process_io_handler.cc
        src/iohandler/process_io_handler.h
        src/layout/fallback_layout.cc
        src/layout/fallback_layout.h
        src/layout/js_layout.cc
        src/layout/js_layout.h
        src/layout/layout.h
        src/metadata/exiv2_handler.cc
        src/metadata/exiv2_handler.h
        src/metadata/ffmpeg_handler.cc
        src/metadata/ffmpeg_handler.h
        src/metadata/metadata_handler.cc
        src/metadata/metadata_handler.h
        src/metadata/libexif_handler.cc
        src/metadata/libexif_handler.h
        src/metadata/taglib_handler.cc
        src/metadata/taglib_handler.h
        src/metadata/fanart_handler.cc
        src/metadata/fanart_handler.h
        src/metadata/matroska_handler.cc
        src/metadata/matroska_handler.h
        src/onlineservice/atrailers_content_handler.cc
        src/onlineservice/atrailers_content_handler.h
        src/onlineservice/atrailers_service.cc
        src/onlineservice/atrailers_service.h
        src/onlineservice/lastfm_scrobbler.cc
        src/onlineservice/lastfm_scrobbler.h
        src/onlineservice/online_service.cc
        src/onlineservice/online_service.h
        src/onlineservice/online_service_helper.cc
        src/onlineservice/online_service_helper.h
        src/onlineservice/sopcast_content_handler.cc
        src/onlineservice/sopcast_content_handler.h
        src/onlineservice/sopcast_service.cc
        src/onlineservice/sopcast_service.h
        src/request_handler.cc
        src/request_handler.h
        src/scripting/import_script.cc
        src/scripting/import_script.h
        src/scripting/js_functions.cc
        src/scripting/js_functions.h
        src/scripting/playlist_parser_script.cc
        src/scripting/playlist_parser_script.h
        src/scripting/runtime.cc
        src/scripting/runtime.h
        src/scripting/script.cc
        src/scripting/script.h
        src/search_handler.cc
        src/search_handler.h
        src/server.cc
        src/serve_request_handler.cc
        src/serve_request_handler.h
        src/server.h
        src/storage/mysql/mysql_create_sql.h
        src/storage/mysql/mysql_storage.cc
        src/storage/mysql/mysql_storage.h
        src/storage/sqlite3/sqlite3_create_sql.h
        src/storage/sqlite3/sqlite3_storage.cc
        src/storage/sqlite3/sqlite3_storage.h
        src/storage/sql_storage.cc
        src/storage/sql_storage.h
        src/storage/storage.cc
        src/storage/storage.h
        src/subscription_request.cc
        src/subscription_request.h
        src/transcoding/transcode_dispatcher.cc
        src/transcoding/transcode_dispatcher.h
        src/transcoding/transcode_ext_handler.cc
        src/transcoding/transcode_ext_handler.h
        src/transcoding/transcode_handler.h
        src/transcoding/transcoding.cc
        src/transcoding/transcoding.h
        src/transcoding/transcoding_process_executor.cc
        src/transcoding/transcoding_process_executor.h
        src/update_manager.cc
        src/update_manager.h
        src/upnp_cds.cc
        src/upnp_cds.h
        src/upnp_cm.cc
        src/upnp_cm.h
        src/upnp_mrreg.cc
        src/upnp_mrreg.h
        src/upnp_xml.cc
        src/upnp_xml.h
        src/url.cc
        src/url.h
        src/url_request_handler.cc
        src/url_request_handler.h
        src/util/executor.h
        src/util/generic_task.cc
        src/util/generic_task.h
        src/util/jpeg_resolution.cc
        src/util/logger.h
        src/util/mt_inotify.cc
        src/util/mt_inotify.h
        src/util/process.cc
        src/util/process_executor.cc
        src/util/process_executor.h
        src/util/process.h
        src/util/string_converter.cc
        src/util/string_converter.h
        src/util/task_processor.cc
        src/util/task_processor.h
        src/util/thread_executor.cc
        src/util/thread_executor.h
        src/util/timer.cc
        src/util/timer.h
        src/util/tools.cc
        src/util/tools.h
        src/util/upnp_clients.h
        src/util/upnp_clients.cc
        src/util/upnp_headers.h
        src/util/upnp_headers.cc
        src/util/upnp_quirks.h
        src/util/upnp_quirks.cc
        src/util/xml_to_json.cc
        src/util/xml_to_json.h
        src/web/action.cc
        src/web/add.cc
        src/web/add_object.cc
        src/web/auth.cc
        src/web/clients.cc
        src/web/containers.cc
        src/web/directories.cc
        src/web/edit_load.cc
        src/web/edit_save.cc
        src/web/files.cc
        src/web/items.cc
        src/web/pages.cc
        src/web/pages.h
        src/web/remove.cc
        src/web/web_request_handler.cc
        src/web/web_request_handler.h
        src/web/session_manager.cc
        src/web/session_manager.h
        src/web/tasks.cc
        src/web/web_autoscan.cc
        src/web/web_update.cc)

add_library(libgerbera STATIC ${libgerberaFILES})
target_include_directories(libgerbera PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_executable(gerbera src/main.cc)
target_link_libraries(gerbera libgerbera)

# Warnings are nice
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Add the automatically determined parts of the RPATH which point to
# directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# System specific stuff
if (${CMAKE_SYSTEM_NAME} MATCHES ".*(SunOS|Solaris).*")
    add_definitions(-DSOLARIS)
elseif (${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
    add_definitions(-DBSD)
endif()

add_definitions(-DPACKAGE_NAME="${PROJECT_NAME}")
add_definitions(-DVERSION="${GERBERA_VERSION}")
add_definitions(-DPACKAGE_DATADIR="${CMAKE_INSTALL_PREFIX}/share/gerbera")

# Generate Compile Information
include("GenCompileInfo")
generate_compile_info()
target_compile_definitions(gerbera PRIVATE -DCOMPILE_INFO="${COMPILE_INFO}")
target_compile_definitions(gerbera PRIVATE -DGIT_BRANCH="${GIT_BRANCH}")
target_compile_definitions(gerbera PRIVATE -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

# Check for C++17 STL Optional
include(CheckIncludeFileCXX)
check_include_file_cxx(optional HAS_STL_OPTIONAL)
if (NOT HAS_STL_OPTIONAL)
    message(FATAL_ERROR "gerbera requires C++17 Optional to be available. Check your compiler is >=GCC 7.1 or >=libc++ 5")
endif()

if (WITH_DEBUG)
    add_definitions(-DTOMBDEBUG)
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
else()
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()

find_package(Threads REQUIRED)
target_link_libraries(libgerbera ${CMAKE_THREAD_LIBS_INIT})

check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_ATOMIC)
if(HAVE_ATOMIC)
    target_link_libraries(libgerbera atomic)
endif(HAVE_ATOMIC)

find_package(Filesystem REQUIRED)
target_link_libraries(libgerbera std::filesystem)

find_package(Iconv REQUIRED)
target_link_libraries(libgerbera Iconv::Iconv)

find_package(libuuid REQUIRED)
include_directories(${libuuid_INCLUDE_DIRS})
target_link_libraries(libgerbera ${libuuid_LIBRARIES})
if (FOUND_BSD_UUID)
    add_definitions(-DBSD_NATIVE_UUID)
endif()

find_package(LFS REQUIRED)
add_definitions(${LFS_DEFINITIONS})
add_compile_options(${LFS_COMPILE_OPTIONS})
target_link_libraries(libgerbera ${LFS_LIBRARIES})

find_package (LibUpnp REQUIRED)
if (UPNP_VERSION_STRING VERSION_LESS "1.12.1")
    message(FATAL_ERROR "gerbera requires libupnp 1.12.1 or above.")
endif()

if (NOT UPNP_HAS_IPV6)
    message(FATAL_ERROR "Gerbera requires libupnp with IPv6 support.")
endif()

if (NOT UPNP_HAS_REUSEADDR)
    message(WARNING "\n !! It is strongly recommended to build libupnp with --enable-reuseaddr !!\nWithout this option Gerbera will be unable to restart with the same port number.")
endif()

target_include_directories(libgerbera PUBLIC ${UPNP_INCLUDE_DIRS})
target_link_libraries (libgerbera ${UPNP_LIBRARIES})

find_package(fmt REQUIRED)
target_link_libraries(libgerbera fmt::fmt)

find_package(spdlog)
if(spdlog_FOUND)
    target_link_libraries(libgerbera spdlog::spdlog)

    if(NOT SPDLOG_FMT_EXTERNAL AND NOT spdlog_COMPILE_DEFINITIONS MATCHES "SPDLOG_FMT_EXTERNAL")
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
        target_link_libraries(libgerbera ${SPDLOG_LIBRARY})
    else()
        message(FATAL_ERROR "spdlog library is required")
    endif()
endif()

find_package(pugixml REQUIRED)
target_include_directories(libgerbera PUBLIC ${pugixml_INCLUDE_DIRS})
target_link_libraries(libgerbera ${pugixml_LIBRARIES})

find_package (SQLite3 REQUIRED)
target_include_directories(libgerbera PUBLIC ${SQLite3_INCLUDE_DIRS})
target_link_libraries (libgerbera ${SQLite3_LIBRARIES})
add_definitions(-DHAVE_SQLITE3)

# We should probably move these two out to their own FindLocale
include(CheckFunctionExists)
check_function_exists(nl_langinfo HAVE_NL_LANGINFO)
if (HAVE_NL_LANGINFO)
    add_definitions("-DHAVE_NL_LANGINFO")
endif()
check_function_exists(setlocale HAVE_SETLOCALE)
if (HAVE_SETLOCALE)
    add_definitions("-DHAVE_SETLOCALE")
endif()

# Link to the socket library if it exists. This is something you need on Solaris/OmniOS/Joyent
find_library(SOCKET_LIBRARY socket)
if(SOCKET_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${SOCKET_LIBRARY})
    target_link_libraries(libgerbera ${SOCKET_LIBRARY})
endif()

# Link to libnsl (Network services library) if it exists. This is something you need on Solaris/OmniOS/Joyent
find_library(NSL_LIBRARY nsl)
if(NSL_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${NSL_LIBRARY})
    target_link_libraries(libgerbera ${NSL_LIBRARY})
endif()

if(WITH_INOTIFY)
    find_package(INotify)
    if(INOTIFY_FOUND)
        include_directories(${INOTIFY_INCLUDE_DIR})
        add_definitions(-DHAVE_INOTIFY)
        # FreeBSD INotify shim!
        if(INOTIFY_LIBRARY)
            target_link_libraries(libgerbera ${INOTIFY_LIBRARY})
        endif()
    endif ()
endif()

if(WITH_JS)
    find_package(Duktape)
    IF (DUKTAPE_FOUND)
        include_directories(${DUKTAPE_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${DUKTAPE_LIBRARIES})
        add_definitions(-DHAVE_JS)
    else()
        message(FATAL_ERROR "Duktape not found!")
    endif()
endif()

if(WITH_MYSQL)
    find_package(MySQL REQUIRED)
    if (MYSQL_FOUND)
        include_directories(${MYSQL_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${MYSQL_CLIENT_LIBS})
        add_definitions(-DHAVE_MYSQL)
    else()
        message(FATAL_ERROR "MySQL not found")
    endif ()
endif()

if(WITH_CURL)
    find_package(PkgConfig QUIET)
    pkg_check_modules (CURL QUIET libcurl)
    if (NOT CURL_FOUND)
        find_package (CURL REQUIRED)
    endif()
    if (CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${CURL_LIBRARIES})

        add_definitions(-DHAVE_CURL)
        add_definitions(-DONLINE_SERVICES)
        add_definitions(-DATRAILERS)
        add_definitions(-DSOPCAST)
    else()
        message(FATAL_ERROR "cURL not found")
    endif()
endif()

if(WITH_TAGLIB)
    find_package (Taglib REQUIRED)
    include_directories(${TAGLIB_INCLUDE_DIRS})
    target_link_libraries (libgerbera ${TAGLIB_LIBRARIES})
    add_definitions(-DHAVE_TAGLIB)
endif()

if(WITH_MAGIC)
    find_package (LibMagic REQUIRED)
    include_directories(${MAGIC_INCLUDE_DIRS})
    target_link_libraries (libgerbera ${MAGIC_LIBRARIES})
    add_definitions(-DHAVE_MAGIC)
endif()

if(WITH_AVCODEC)
    find_package (FFMPEG)
    if (FFMPEG_FOUND)
        include_directories(${FFMPEG_INCLUDE_DIR})
        target_link_libraries (libgerbera ${FFMPEG_LIBRARIES})
        add_definitions(-DHAVE_FFMPEG)
        if (HAVE_AVSTREAM_CODECPAR)
            add_definitions(-DHAVE_AVSTREAM_CODECPAR)
        endif()

    else()
        message(FATAL_ERROR "FFMpeg/LibAV not found")
    endif ()
endif()

if(WITH_FFMPEGTHUMBNAILER)
    find_package (FFMpegThumbnailer)
    if (FFMPEGTHUMBNAILER_FOUND)
        include_directories(${FFMPEGTHUMBNAILER_INCLUDE_DIR})
        target_link_libraries (libgerbera ${FFMPEGTHUMBNAILER_LIBRARIES})
        add_definitions(-DHAVE_FFMPEGTHUMBNAILER)
    else()
        message(FATAL_ERROR "FFMpegThumbnailer not found")
    endif ()
endif()

if(WITH_EXIF)
    find_package (EXIF REQUIRED)
    include_directories(${EXIF_INCLUDE_DIRS})
    target_link_libraries (libgerbera ${EXIF_LIBRARIES})
    add_definitions(-DHAVE_LIBEXIF)
endif()

if(WITH_EXIV2)
    find_package (EXIV2)
    if (EXIV2_FOUND)
        include_directories(${EXIV2_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${EXIV2_LIBRARIES})
        add_definitions(-DHAVE_EXIV2)
        # Exiv still uses std::auto_ptr
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_definitions(-D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR)
        endif()
    else()
        message(FATAL_ERROR "Exiv2 not found")
    endif()
endif()

if(WITH_MATROSKA)
    find_package (Matroska)
    if (MATROSKA_FOUND AND EBML_FOUND)
        include_directories(${MATROSKA_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${MATROSKA_LIBRARIES})
        include_directories(${EBML_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${EBML_LIBRARIES})
        add_definitions(-DHAVE_MATROSKA)
    else()
        message(FATAL_ERROR "Matroska not found")
    endif()
endif()

if(WITH_LASTFM)
    find_package (LastFMLib)
    if (LASTFMLIB_FOUND)
        include_directories(${LASTFMLIB_INCLUDE_DIRS})
        target_link_libraries (libgerbera ${LASTFMLIB_LIBRARIES})
        add_definitions(-DHAVE_LASTFMLIB)
    else()
        message(FATAL_ERROR "LastFMLib not found")
    endif()
endif()

if(WITH_SYSTEMD)
    find_package(Systemd)
    if(SYSTEMD_FOUND)
        if(WITH_MYSQL AND MYSQL_FOUND)
            set (SYSTEMD_AFTER_TARGET "mysql.target network.target")
            set (SYSTEMD_DESCRIPTION "Gerbera Media Server with MySQL")
        else()
            set (SYSTEMD_AFTER_TARGET "network.target")
            set (SYSTEMD_DESCRIPTION "Gerbera Media Server")
        endif()

        configure_file(${CMAKE_SOURCE_DIR}/scripts/systemd/gerbera.service.cmake gerbera.service)
        message(STATUS "Configuring systemd unit file: gerbera.service" )
        INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/gerbera.service DESTINATION ${SYSTEMD_UNIT_DIR} COMPONENT init)
    else()
        message(FATAL_ERROR "Systemd not found")
    endif()
endif()

# Needs to be towards the end (after CURL, TagLib, etc...)
find_package (ZLIB REQUIRED)
target_link_libraries (libgerbera ZLIB::ZLIB)

if(WITH_TESTS)
    message(STATUS "Configuring unit tests")
    enable_testing()
    add_subdirectory(test)
endif()

INSTALL(TARGETS gerbera DESTINATION bin)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/js DESTINATION share/gerbera)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/web DESTINATION share/gerbera)
INSTALL(FILES
        ${PROJECT_SOURCE_DIR}/config/mappings.xml
        ${PROJECT_SOURCE_DIR}/config/mysql.sql
        ${PROJECT_SOURCE_DIR}/config/sqlite3.sql
        DESTINATION share/gerbera)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/doc/gerbera.1 DESTINATION share/man/man1)
