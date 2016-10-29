cmake_minimum_required(VERSION 3.6)

set(CMAKE_VERBOSE_MAKEFILE on)

project("MediaTomb")

add_library(libmediatomb OBJECT
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
        src/dvd_io_handler.cc
        src/dvd_io_handler.h
        src/dvdnav_read.cc
        src/dvdnav_read.h
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
        src/hash/dbb_hash.h
        src/hash/db_hash.h
        src/hash/dbo_hash.h
        src/hash/dbr_hash.h
        src/hash/direct_hash_base.h
        src/hash/dsb_hash.h
        src/hash/dso_hash.h
        src/hash.h
        src/inotify-nosys.h
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
        src/metadata/dvd_handler.cc
        src/metadata/dvd_handler.h
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
        src/mpegdemux/buffer.h
        src/mpegdemux/buffer.c
        src/mpegdemux/mpeg_parse.c
        src/mpegdemux/mpeg_parse.h
        src/mpegdemux/mpeg_remux.c
        src/mpegdemux/mpeg_remux.h
        src/mpegdemux/mpegdemux.c
        src/mpegdemux/mpegdemux_internal.h
        src/mpegremux_processor.cc
        src/mpegremux_processor.h
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
        src/scripting/dvd_image_import_script.cc
        src/scripting/dvd_image_import_script.h
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
        src/sync.cc
        src/sync.h
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
        src/weborama_content_handler.cc
        src/weborama_content_handler.h
        src/weborama_service.cc
        src/weborama_service.h
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
        src/zmm/atomic.h
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
        src/zmm/nil.cc
        src/zmm/nil.h
        src/zmm/null.h
        src/zmm/object.cc
        src/zmm/object.h
        src/zmm/ref.h
        src/zmm/stringbuffer.cc
        src/zmm/stringbuffer.h
        src/zmm/strings.cc
        src/zmm/strings.h
        src/zmm/zmm.h
        )

target_include_directories(libmediatomb PRIVATE "${CMAKE_SOURCE_DIR}/src")

add_executable(mediatomb src/main.cc $<TARGET_OBJECTS:libmediatomb>)
target_include_directories(mediatomb PRIVATE "${CMAKE_SOURCE_DIR}/src")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

add_definitions(-DPACKAGE_NAME="MediaTomb-v00d00")
add_definitions(-DVERSION="0.13.0_alpha-v00d00")
add_definitions(-DPACKAGE_DATADIR="${CMAKE_INSTALL_PREFIX}/share/mediatomb")
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX} )

add_definitions(-DCOMPILE_INFO="")
add_definitions(-DEXTERNAL_TRANSCODING)

# This only compiles them in
add_definitions(-DDEBUG_LOG_ENABLED)
add_definitions(-DTOMBDEBUG)
add_definitions(-DLOG_ENABLED)

find_package(Threads REQUIRED)
target_link_libraries (mediatomb ${CMAKE_THREAD_LIBS_INIT})

find_package(INotify REQUIRED)
target_link_libraries (mediatomb ${INOTIFY_LIBRARY})
add_definitions(-DHAVE_INOTIFY)

find_package(LibUUID REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})
target_link_libraries (mediatomb ${LIBUUID_LIBRARIES})

find_package (LibUpnp REQUIRED)
include_directories(${UPNP_INCLUDE_DIRS})
target_link_libraries (mediatomb ${UPNP_LIBRARIES})

find_package (EXPAT REQUIRED)
include_directories(${EXPAT_INCLUDE_DIRS})
target_link_libraries (mediatomb ${EXPAT_LIBRARIES})
add_definitions(-DHAVE_EXPAT)

find_package (SQLite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})
target_link_libraries (mediatomb ${SQLITE3_LIBRARIES})
add_definitions(-DHAVE_SQLITE3)

if(WITH_MYSQL)
    find_package(MySQL REQUIRED)
    if (MYSQL_FOUND)
        include_directories(${MYSQL_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${MYSQL_CLIENT_LIBS})
        add_definitions(-DHAVE_MYSQL)
    endif (MYSQL_FOUND)
endif(WITH_MYSQL)

if(WITH_CURL)
    find_package (CURL REQUIRED)
    if (CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${CURL_LIBRARIES})

        add_definitions(-DHAVE_CURL)
        add_definitions(-DONLINE_SERVICES)
        add_definitions(-DYOUTUBE)
    endif (CURL_FOUND)
endif(WITH_CURL)
    
if(WITH_JS)
    find_package (SpiderMonkey)
    if (JS_FOUND)
        include_directories(${JS_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${JS_LIBRARIES})
        add_definitions(-DHAVE_JS)
    endif (JS_FOUND)
endif(WITH_JS)

if(WITH_TAGLIB)
    find_package (Taglib)
    if (TAGLIB_FOUND)
        include_directories(${TAGLIB_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${TAGLIB_LIBRARIES})
        add_definitions(-DHAVE_TAGLIB)
    endif (TAGLIB_FOUND)
endif(WITH_TAGLIB)

if(WITH_MAGIC)
    find_package (LibMagic)
    if (MAGIC_FOUND)
        include_directories(${MAGIC_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${MAGIC_LIBRARIES})
        add_definitions(-DHAVE_MAGIC)
    endif (MAGIC_FOUND)
endif(WITH_MAGIC)

if(WITH_AVCODEC)
    find_package (LibAV)
    if (LIBAV_FOUND)
        include_directories(${LIBAV_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${LIBAV_LIBRARIES})
        add_definitions(-DHAVE_FFMPEG)
    endif (LIBAV_FOUND)
endif(WITH_AVCODEC)

if(WITH_DVDNAV)
    find_package (DvdNav)
    if (DVDNAV_FOUND)
        include_directories(${DVDNAV_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${DVDNAV_LIBRARIES})
        add_definitions(-DHAVE_LIBDVDNAV)
    endif (DVDNAV_FOUND)
endif(WITH_DVDNAV)

if(WITH_EXIF)
    find_package (EXIF)
    if (EXIF_FOUND)
        include_directories(${EXIF_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${EXIF_LIBRARIES})
        add_definitions(-DHAVE_LIBEXIF)
    endif (EXIF_FOUND)
endif(WITH_EXIF)

if(WITH_LASTFM)
    find_package (LastFMLib)
    if (LASTFMLIB_FOUND)
        include_directories(${LASTFMLIB_INCLUDE_DIRS})
        target_link_libraries (mediatomb ${LASTFMLIB_LIBRARIES})
        add_definitions(-DHAVE_LASTFMLIB)
    endif (LASTFMLIB_FOUND)
endif(WITH_LASTFM)

INSTALL(TARGETS mediatomb DESTINATION bin)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/web DESTINATION share/mediatomb)
INSTALL(FILES
        ${PROJECT_SOURCE_DIR}/config/mappings.xml
        ${PROJECT_SOURCE_DIR}/config/mysql.sql
        ${PROJECT_SOURCE_DIR}/config/sqlite3.sql
        DESTINATION share/mediatomb)
