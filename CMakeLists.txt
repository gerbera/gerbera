cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# It is 2017 so lets leverage a modern C++
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CheckFunctionExists)

set(CMAKE_VERBOSE_MAKEFILE off CACHE BOOL "Show verbose build commands")
set(WITH_MAGIC          1 CACHE BOOL "Use libmagic to identify file mime types")
set(WITH_MYSQL          0 CACHE BOOL "Store media information in MySQL DB")
set(WITH_CURL           1 CACHE BOOL "CURL required for YouTube and/or SopCast")
set(WITH_INOTIFY        1 CACHE BOOL "Enable Inotify file monitoring support")
set(WITH_JS             1 CACHE BOOL "Enable JavaScript for custom import script")
set(WITH_TAGLIB         1 CACHE BOOL "Use TagLib to extract audio file metadata")
set(WITH_AVCODEC        0 CACHE BOOL "Enable ffmpeg/libav")
set(WITH_FFMPEGTHUMBNAILER 0 CACHE BOOL "Enable Thumbnail generation")
set(WITH_EXIF           1 CACHE BOOL "Use libexif to extract image metadata")
set(WITH_LASTFM         0 CACHE BOOL "Enable LastFM")
set(WITH_LOGGING        1 CACHE BOOL "Enable file logging")
set(WITH_DEBUG_LOGGING  1 CACHE BOOL "Enables debug logging")

project("gerbera")
set(gerbera_MAJOR_VERSION     1)
set(gerbera_MINOR_VERSION     1)
set(gerbera_PATCH_VERSION     0)
set(gerbera_RELEASE           "_alpha")
set(gerbera_SUFFIX            "")

set(libgerberaFILES
        src/action_request.cc
        src/action_request.h
        src/atrailers_content_handler.cc
        src/atrailers_content_handler.h
        src/atrailers_service.cc
        src/atrailers_service.h
        src/autoscan.cc
        src/autoscan.h
        src/autoscan_inotify.cc
        src/autoscan_inotify.h
        src/buffered_io_handler.cc
        src/buffered_io_handler.h
        src/cached_url.cc
        src/cached_url.h
        src/cds_objects.cc
        src/cds_objects.h
        src/cds_resource.cc
        src/cds_resource.h
        src/cds_resource_manager.cc
        src/cds_resource_manager.h
        src/common.h
        src/config_manager.cc
        src/config_manager.h
        src/config_options.h
        src/content_manager.cc
        src/content_manager.h
        src/curl_io_handler.cc
        src/curl_io_handler.h
        src/destroyer.cc
        src/destroyer.h
        src/dictionary.cc
        src/dictionary.h
        src/exceptions.cc
        src/exceptions.h
        src/executor.h
        src/fd_io_handler.cc
        src/fd_io_handler.h
        src/file_io_handler.cc
        src/file_io_handler.h
        src/file_request_handler.cc
        src/file_request_handler.h
        src/filesystem.cc
        src/filesystem.h
        src/generic_task.cc
        src/generic_task.h
        src/io_handler_buffer_helper.cc
        src/io_handler_buffer_helper.h
        src/io_handler.cc
        src/io_handler_chainer.cc
        src/io_handler_chainer.h
        src/io_handler.h
        src/jpeg_resolution.cc
        src/lastfm_scrobbler.cc
        src/lastfm_scrobbler.h
        src/layout/fallback_layout.cc
        src/layout/fallback_layout.h
        src/layout/js_layout.cc
        src/layout/js_layout.h
        src/layout/layout.h
        src/logger.cc
        src/logger.h
        src/md5/md5.c
        src/md5/md5.h
        src/mem_io_handler.cc
        src/mem_io_handler.h
        src/memory.cc
        src/memory.h
        src/metadata/exiv2_handler.cc
        src/metadata/exiv2_handler.h
        src/metadata/ffmpeg_handler.cc
        src/metadata/ffmpeg_handler.h
        src/metadata_handler.cc
        src/metadata_handler.h
        src/metadata/libexif_handler.cc
        src/metadata/libexif_handler.h
        src/metadata/taglib_handler.cc
        src/metadata/taglib_handler.h
        src/metadata/fanart_handler.cc
        src/metadata/fanart_handler.h
        src/mt_inotify.cc
        src/mt_inotify.h
        src/mxml/attribute.cc
        src/mxml/attribute.h
        src/mxml/comment.cc
        src/mxml/comment.h
        src/mxml/context.cc
        src/mxml/context.h
        src/mxml/document.cc
        src/mxml/document.h
        src/mxml/element.cc
        src/mxml/element.h
        src/mxml/mxml.h
        src/mxml/node.cc
        src/mxml/node.h
        src/mxml/parseexception.cc
        src/mxml/parseexception.h
        src/mxml/parser_expat.cc
        src/mxml/parser.h
        src/mxml/xml_text.cc
        src/mxml/xml_text.h
        src/mxml/xml_to_json.cc
        src/mxml/xml_to_json.h
        src/object_dictionary.h
        src/online_service.cc
        src/online_service.h
        src/online_service_helper.cc
        src/online_service_helper.h
        src/play_hook.cc
        src/play_hook.h
        src/process.cc
        src/process_executor.cc
        src/process_executor.h
        src/process.h
        src/process_io_handler.cc
        src/process_io_handler.h
        src/reentrant_array.h
        src/request_handler.cc
        src/request_handler.h
        src/rexp.cc
        src/rexp.h
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
        src/server.cc
        src/serve_request_handler.cc
        src/serve_request_handler.h
        src/server.h
        src/session_manager.cc
        src/session_manager.h
        src/singleton.cc
        src/singleton.h
        src/sopcast_content_handler.cc
        src/sopcast_content_handler.h
        src/sopcast_service.cc
        src/sopcast_service.h
        src/storage/cache_object.cc
        src/storage/cache_object.h
        src/storage.cc
        src/storage.h
        src/storage/mysql/mysql_create_sql.h
        src/storage/mysql/mysql_storage.cc
        src/storage/mysql/mysql_storage.h
        src/storage/sqlite3/sqlite3_create_sql.h
        src/storage/sqlite3/sqlite3_storage.cc
        src/storage/sqlite3/sqlite3_storage.h
        src/storage/sql_storage.cc
        src/storage/sql_storage.h
        src/storage/storage_cache.cc
        src/storage/storage_cache.h
        src/string_converter.cc
        src/string_converter.h
        src/subscription_request.cc
        src/subscription_request.h
        src/task_processor.cc
        src/task_processor.h
        src/thread_executor.cc
        src/thread_executor.h
        src/timer.cc
        src/timer.h
        src/tools.cc
        src/tools.h
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
        src/upnp_cds_actions.cc
        src/upnp_cds.cc
        src/upnp_cds.h
        src/upnp_cds_subscriptions.cc
        src/upnp_cm_actions.cc
        src/upnp_cm.cc
        src/upnp_cm.h
        src/upnp_cm_subscriptions.cc
        src/upnp_mrreg_actions.cc
        src/upnp_mrreg.cc
        src/upnp_mrreg.h
        src/upnp_mrreg_subscriptions.cc
        src/upnp_xml.cc
        src/upnp_xml.h
        src/url.cc
        src/url.h
        src/url_request_handler.cc
        src/url_request_handler.h
        src/web/action.cc
        src/web/add.cc
        src/web/add_object.cc
        src/web/auth.cc
        src/web_callbacks.cc
        src/web_callbacks.h
        src/web/containers.cc
        src/web/directories.cc
        src/web/edit_load.cc
        src/web/edit_save.cc
        src/web/files.cc
        src/web/items.cc
        src/web/pages.cc
        src/web/pages.h
        src/web/remove.cc
        src/web_request_handler.cc
        src/web_request_handler.h
        src/web/tasks.cc
        src/web/web_autoscan.cc
        src/web/web_update.cc
        src/xpath.cc
        src/xpath.h
        src/youtube_content_handler.cc
        src/youtube_content_handler.h
        src/youtube_service.cc
        src/youtube_service.h
        src/youtube_video_url.cc
        src/youtube_video_url.h
        src/zmm/array.cc
        src/zmm/array.h
        src/zmm/base_array.h
        src/zmm/base_queue.h
        src/zmm/base_stack.h
        src/zmm/exception.cc
        src/zmm/exception.h
        src/zmm/object_queue.h
        src/zmm/object_stack.h
        src/zmm/stringtokenizer.cc
        src/zmm/stringtokenizer.h
        src/zmm/zmmf.h
        src/zmm/object.cc
        src/zmm/object.h
        src/zmm/ref.h
        src/zmm/stringbuffer.cc
        src/zmm/stringbuffer.h
        src/zmm/strings.cc
        src/zmm/strings.h
        src/zmm/zmm.h)

add_library(libgerbera OBJECT ${libgerberaFILES})

target_include_directories(libgerbera PRIVATE "${CMAKE_SOURCE_DIR}/src")

add_executable(gerbera src/main.cc $<TARGET_OBJECTS:libgerbera>)
target_include_directories(gerbera PRIVATE "${CMAKE_SOURCE_DIR}/src")

# Warnings are nice
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Add the automatically determined parts of the RPATH which point to
# dirss outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Provide __FILENAME__ for debug statements
include(DefFileName)
define_file_path_for_sources(libgerbera)
define_file_path_for_sources(gerbera)

# Use libc++ with clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# Host OS Check
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(HOST_OS "linux")
    set(LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES ".*(SunOS|Solaris).*")
    set(HOST_OS "solaris")
    set(SOLARIS 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
    set(HOST_OS "bsd")
    set(BSD 1)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(HOST_OS "darwin")
    set(DARWIN 1)
endif()
if(NOT HOST_OS)
    message(FATAL_ERROR 
                "gerbera was unable to deterimine the host OS. Please report this. Value of CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
endif()

if (SOLARIS)
    add_definitions(-DSOLARIS)
endif()

add_definitions(-DPACKAGE_NAME="gerbera${gerbera_SUFFIX}")
add_definitions(-DVERSION="${gerbera_MAJOR_VERSION}.${gerbera_MINOR_VERSION}.${gerbera_PATCH_VERSION}${gerbera_RELEASE}${gerbera_SUFFIX}")
add_definitions(-DPACKAGE_DATADIR="${CMAKE_INSTALL_PREFIX}/share/gerbera")

add_definitions(-DCOMPILE_INFO="")
add_definitions(-DEXTERNAL_TRANSCODING -DAUTO_CREATE_DATABASE)

if (WITH_LOGGING OR WITH_DEBUG_LOGGING)
    add_definitions(-DLOG_ENABLED)
endif()
if (WITH_DEBUG_LOGGING)
    add_definitions(-DTOMBDEBUG)
endif()

find_package(Threads REQUIRED)
target_link_libraries (gerbera ${CMAKE_THREAD_LIBS_INIT})

find_package(Iconv REQUIRED)
include_directories(${ICONV_INCLUDE_DIR})
target_link_libraries (gerbera ${ICONV_LIBRARIES})

find_package(LibUUID REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})
target_link_libraries (gerbera ${LIBUUID_LIBRARIES})

find_package (LibUpnp REQUIRED)
include_directories(${UPNP_INCLUDE_DIRS})
target_link_libraries (gerbera ${UPNP_LIBRARIES})

if (UPNP_VERSION_STRING VERSION_LESS "1.8.0")
    message(FATAL_ERROR "gerbera requires libupnp 1.8.0 or above.")
endif()
if (NOT UPNP_HAS_IPV6)
    message(FATAL_ERROR "gerbera requires libupnp with IPv6 support.")
endif()
if (NOT UPNP_HAS_REUSEADDR)
    message(WARNING "\n !! It is strongly recommended to build libupnp with --enable-reuseaddr !!\nWithout this option Gerbera will be unable to restart with the same port number.")
endif()

# Debian ships a snapshot from 2010: libupnp4 (1.8.0~svn20100507-1.2)
# This is missing templated types so we need to ifdef
if(UPNP_INCLUDE_DIR AND NOT EXISTS "${UPNP_INCLUDE_DIR}/upnp-1.8/TemplateInclude.h")
    message (WARNING "\n!! You are using a very old 1.8 snapshot. Please upgrade to a newer snapshot from upstream (https://github.com/mrjimenez/pupnp) !!\n")
    message (STATUS "gerbera: Enabling old libupnp snapshot compat. Web interface will probably not work.")
    add_definitions(-DUPNP_OLD_SNAPSHOT)
endif()

find_package (EXPAT REQUIRED)
if (EXPAT_FOUND)
    include_directories(${EXPAT_INCLUDE_DIRS})
    target_link_libraries (gerbera ${EXPAT_LIBRARIES})
else()
    message(FATAL_ERROR "Expat not found!")
endif()

find_package (SQLite3 REQUIRED)
if(SQLITE3_FOUND)
    include_directories(${SQLITE3_INCLUDE_DIRS})
    target_link_libraries (gerbera ${SQLITE3_LIBRARIES})
    add_definitions(-DHAVE_SQLITE3)
else()
    message(FATAL_ERROR "SQLite3 not found!")
endif()

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
    target_link_libraries(gerbera ${SOCKET_LIBRARY})
endif()

# Link to libnsl (Network services library) if it exists. This is something you need on Solaris/OmniOS/Joyent
find_library(NSL_LIBRARY nsl)
if(NSL_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${NSL_LIBRARY})
    target_link_libraries(gerbera ${NSL_LIBRARY})
endif()

if(WITH_INOTIFY)
    find_package(INotify)
    if(INOTIFY_FOUND)
        include_directories(${INOTIFY_INCLUDE_DIR})
        add_definitions(-DHAVE_INOTIFY)
        # FreeBSD INotify shim!
        if(LIBINOTIFY_LIBRARY)
            target_link_libraries(gerbera ${LIBINOTIFY_LIBRARY})
        endif()
    endif ()
endif()

if(WITH_JS)
    find_package(Duktape)
    IF (DUKTAPE_FOUND)
        include_directories(${DUKTAPE_INCLUDE_DIRS})
        target_link_libraries (gerbera ${DUKTAPE_LIBRARIES})
        add_definitions(-DHAVE_JS)
    else()
        message(FATAL_ERROR "Duktape not found!")
    endif()
endif()

if(WITH_MYSQL)
    find_package(MySQL REQUIRED)
    if (MYSQL_FOUND)
        include_directories(${MYSQL_INCLUDE_DIRS})
        target_link_libraries (gerbera ${MYSQL_CLIENT_LIBS})
        add_definitions(-DHAVE_MYSQL)
    else()
        message(FATAL_ERROR "MySQL not found")
    endif ()
endif()

if(WITH_CURL)
    find_package (CURL REQUIRED)
    if (CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
        target_link_libraries (gerbera ${CURL_LIBRARIES})

        add_definitions(-DHAVE_CURL)
        add_definitions(-DONLINE_SERVICES)
        add_definitions(-DYOUTUBE)
    else()
        message(FATAL_ERROR "cURL not found")
    endif()
endif()

if(WITH_TAGLIB)
    find_package (Taglib)
    if (TAGLIB_FOUND)
        include_directories(${TAGLIB_INCLUDE_DIRS})
        target_link_libraries (gerbera ${TAGLIB_LIBRARIES})
        add_definitions(-DHAVE_TAGLIB)
    else()
        message(FATAL_ERROR "Taglib not found!")
    endif()
endif()

if(WITH_MAGIC)
    find_package (LibMagic)
    if (MAGIC_FOUND)
        include_directories(${MAGIC_INCLUDE_DIRS})
        target_link_libraries (gerbera ${MAGIC_LIBRARIES})
        add_definitions(-DHAVE_MAGIC)
    else()
        message(FATAL_ERROR "Magic not found")
    endif ()
endif()

if(WITH_AVCODEC)
    find_package (FFMPEG)
    if (FFMPEG_FOUND)
        include_directories(${FFMPEG_INCLUDE_DIR})
        target_link_libraries (gerbera ${FFMPEG_LIBRARIES})
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
        target_link_libraries (gerbera ${FFMPEGTHUMBNAILER_LIBRARIES})
        add_definitions(-DHAVE_FFMPEGTHUMBNAILER)
    else()
        message(FATAL_ERROR "FFMpegThumbnailer not found")
    endif ()
endif()

if(WITH_EXIF)
    find_package (EXIF)
    if (EXIF_FOUND)
        include_directories(${EXIF_INCLUDE_DIRS})
        target_link_libraries (gerbera ${EXIF_LIBRARIES})
        add_definitions(-DHAVE_LIBEXIF)
    else()
        message(FATAL_ERROR "LibExif not found")
    endif()
endif()

if(WITH_LASTFM)
    find_package (LastFMLib)
    if (LASTFMLIB_FOUND)
        include_directories(${LASTFMLIB_INCLUDE_DIRS})
        target_link_libraries (gerbera ${LASTFMLIB_LIBRARIES})
        add_definitions(-DHAVE_LASTFMLIB)
    else()
        message(FATAL_ERROR "LastFMLib not found")
    endif()
endif()

# Needs to be towards the end (after CURL, TagLib, etc...)
find_package (ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries (gerbera ${ZLIB_LIBRARIES})

INSTALL(TARGETS gerbera DESTINATION bin)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/js DESTINATION share/gerbera)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/web DESTINATION share/gerbera)
INSTALL(FILES
        ${PROJECT_SOURCE_DIR}/config/mappings.xml
        ${PROJECT_SOURCE_DIR}/config/mysql.sql
        ${PROJECT_SOURCE_DIR}/config/sqlite3.sql
        DESTINATION share/gerbera)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/doc/gerbera.1 DESTINATION share/man/man1)
