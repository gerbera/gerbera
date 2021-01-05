cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
cmake_policy(VERSION 3.13)

set(GERBERA_VERSION 1.7.0_git)
project(Gerbera)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

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

set(WITH_NPUPNP         0 CACHE BOOL "Use npupnp instead of pupnp")
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

add_library(libgerbera STATIC 
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
        src/config/config_options.cc
        src/config/config_options.h
        src/config/config_setup.cc
        src/config/config_setup.h
        src/config/directory_tweak.h
        src/config/directory_tweak.cc
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
        src/metadata/metacontent_handler.cc
        src/metadata/metacontent_handler.h
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
        src/scripting/script_names.h
        src/search_handler.cc
        src/search_handler.h
        src/server.cc
        src/serve_request_handler.cc
        src/serve_request_handler.h
        src/server.h
        src/database/mysql/mysql_database.cc
        src/database/mysql/mysql_database.h
        src/database/sqlite3/sqlite_database.cc
        src/database/sqlite3/sqlite_database.h
        src/database/sql_database.cc
        src/database/sql_database.h
        src/database/database.cc
        src/database/database.h
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
        src/util/mime.cc
        src/util/mime.h
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
        src/web/config_load.cc
        src/web/config_save.cc
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
        src/web/web_update.cc
        )
target_include_directories(libgerbera PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_executable(gerbera src/main.cc)
target_link_libraries(gerbera libgerbera)

# Leverage a modern C++
target_compile_features(gerbera PUBLIC cxx_std_17)
set_target_properties(gerbera PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS NO)

# Warnings are nice
target_compile_options(gerbera PRIVATE -Wall)
target_compile_options(libgerbera PRIVATE -Wall)

# Add the automatically determined parts of the RPATH which point to
# directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# System specific stuff
if (${CMAKE_SYSTEM_NAME} MATCHES ".*(SunOS|Solaris).*")
    target_compile_definitions(libgerbera PUBLIC SOLARIS)
elseif (${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
    target_compile_definitions(libgerbera PUBLIC BSD)
endif()

target_compile_definitions(libgerbera PUBLIC GERBERA_VERSION="${GERBERA_VERSION}")
target_compile_definitions(gerbera PRIVATE PACKAGE_DATADIR="${CMAKE_INSTALL_PREFIX}/share/gerbera")

# Generate Compile Information
include("GenCompileInfo")
generate_compile_info()
target_compile_definitions(gerbera PRIVATE COMPILE_INFO="${COMPILE_INFO}")
target_compile_definitions(gerbera PRIVATE GIT_BRANCH="${GIT_BRANCH}")
target_compile_definitions(gerbera PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

# Check for C++17 STL Optional
include(CheckIncludeFileCXX)
check_include_file_cxx(optional HAS_STL_OPTIONAL)
if (NOT HAS_STL_OPTIONAL)
    message(FATAL_ERROR "gerbera requires C++17 Optional to be available. Check your compiler is >=GCC 7.1 or >=libc++ 5")
endif()

if (WITH_DEBUG)
    target_compile_definitions(libgerbera PUBLIC TOMBDEBUG)
    target_compile_definitions(libgerbera PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
else()
    target_compile_definitions(libgerbera PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()

find_package(Threads REQUIRED)
target_link_libraries(libgerbera PUBLIC Threads::Threads)

check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_ATOMIC)
if(HAVE_ATOMIC)
    target_link_libraries(libgerbera PUBLIC atomic)
endif(HAVE_ATOMIC)

find_package(Filesystem REQUIRED)
target_link_libraries(libgerbera PUBLIC std::filesystem)

find_package(Iconv REQUIRED)
target_link_libraries(libgerbera PUBLIC Iconv::Iconv)

find_package(libuuid REQUIRED)
target_include_directories(libgerbera PUBLIC ${libuuid_INCLUDE_DIRS})
target_link_libraries(libgerbera PUBLIC ${libuuid_LIBRARIES})
if (FOUND_BSD_UUID)
    target_compile_definitions(libgerbera PUBLIC BSD_NATIVE_UUID)
endif()

find_package(LFS REQUIRED)
target_compile_definitions(libgerbera PUBLIC ${LFS_DEFINITIONS})
target_compile_options(libgerbera PUBLIC ${LFS_COMPILE_OPTIONS})
target_link_libraries(libgerbera PUBLIC ${LFS_LIBRARIES})

if (WITH_NPUPNP)
    find_package(PkgConfig QUIET)
    pkg_check_modules(NPUPNP QUIET libnpupnp>=4.0.11)
    if (NOT NPUPNP_FOUND)
        find_package(NPUPNP REQUIRED)
    endif()
    target_include_directories(libgerbera PUBLIC ${NPUPNP_INCLUDE_DIRS})
    target_compile_definitions(libgerbera PUBLIC USING_NPUPNP)
    target_link_libraries(libgerbera PUBLIC ${NPUPNP_LIBRARIES})
else()
    find_package (pupnp "1.12.1" REQUIRED)
    include(CheckCXXSymbolExists)

    get_target_property(UPNP_INCLUDE_DIR pupnp::pupnp INTERFACE_INCLUDE_DIRECTORIES)
    check_cxx_symbol_exists(UPNP_ENABLE_IPV6 "${UPNP_INCLUDE_DIR}/upnpconfig.h" UPNP_HAS_IPV6)
    if (NOT UPNP_HAS_IPV6)
        message(FATAL_ERROR "Gerbera requires libupnp with IPv6 support.")
    endif()

    check_cxx_symbol_exists(UPNP_MINISERVER_REUSEADDR "${UPNP_INCLUDE_DIR}/upnpconfig.h" UPNP_HAS_REUSEADDR)
    if (NOT UPNP_HAS_REUSEADDR)
        message(WARNING [=[
    !! It is strongly recommended to build libupnp with --enable-reuseaddr !!
    Without this option Gerbera will be unable to restart with the same port number.]=])
    endif()

    target_link_libraries(libgerbera PUBLIC pupnp::pupnp)
endif()

find_package(fmt REQUIRED)
target_link_libraries(libgerbera PUBLIC fmt::fmt)

find_package(spdlog REQUIRED)
target_link_libraries(libgerbera PUBLIC spdlog::spdlog)

find_package(pugixml REQUIRED)
target_link_libraries(libgerbera PUBLIC pugixml::pugixml)

find_package(SQLite3 REQUIRED)
# Unfortunately Conan and CMake Find module produce slightly different targets.
if(TARGET SQLite::SQLite3)
    target_link_libraries(libgerbera PUBLIC SQLite::SQLite3)
else()
    target_link_libraries(libgerbera PUBLIC SQLite3::SQLite3)
endif()

# We should probably move these two out to their own FindLocale
include(CheckFunctionExists)
check_function_exists(nl_langinfo HAVE_NL_LANGINFO)
if (HAVE_NL_LANGINFO)
    target_compile_definitions(libgerbera PUBLIC HAVE_NL_LANGINFO)
endif()
check_function_exists(setlocale HAVE_SETLOCALE)
if (HAVE_SETLOCALE)
    target_compile_definitions(libgerbera PUBLIC HAVE_SETLOCALE)
endif()

# Link to the socket library if it exists. This is something you need on Solaris/OmniOS/Joyent
find_library(SOCKET_LIBRARY socket)
if(SOCKET_LIBRARY)
    target_link_libraries(libgerbera PUBLIC ${SOCKET_LIBRARY})
endif()

# Link to libnsl (Network services library) if it exists. This is something you need on Solaris/OmniOS/Joyent
find_library(NSL_LIBRARY nsl)
if(NSL_LIBRARY)
    target_link_libraries(libgerbera PUBLIC ${NSL_LIBRARY})
endif()

if(WITH_INOTIFY)
    find_package(INotify REQUIRED)
    if(INOTIFY_FOUND)
        target_include_directories(libgerbera PUBLIC ${INOTIFY_INCLUDE_DIR})
        target_compile_definitions(libgerbera PUBLIC HAVE_INOTIFY)
        # FreeBSD INotify shim!
        if(INOTIFY_LIBRARY)
            target_link_libraries(libgerbera PUBLIC ${INOTIFY_LIBRARY})
        endif()
    endif()
endif()

if(WITH_JS)
    find_package(duktape REQUIRED)
    target_link_libraries(libgerbera PUBLIC duktape::duktape)
    target_compile_definitions(libgerbera PUBLIC HAVE_JS)
endif()

if(WITH_MYSQL)
    find_package(MySQL REQUIRED)
    target_include_directories(libgerbera PUBLIC ${MYSQL_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${MYSQL_CLIENT_LIBS})
    target_compile_definitions(libgerbera PUBLIC HAVE_MYSQL)
endif()

if(WITH_CURL)
    find_package(PkgConfig QUIET)
    pkg_check_modules(CURL QUIET libcurl)
    if (NOT CURL_FOUND)
        find_package(CURL REQUIRED)
    endif()

    target_include_directories(libgerbera PUBLIC ${CURL_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${CURL_LIBRARIES})

    target_compile_definitions(libgerbera PUBLIC
        HAVE_CURL
        ONLINE_SERVICES
        ATRAILERS
        SOPCAST)
endif()

if(WITH_TAGLIB)
    find_package(Taglib REQUIRED)
    target_include_directories(libgerbera PUBLIC ${TAGLIB_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${TAGLIB_LIBRARIES})
    target_compile_definitions(libgerbera PUBLIC HAVE_TAGLIB)
endif()

if(WITH_MAGIC)
    find_package(LibMagic REQUIRED)
    target_include_directories(libgerbera PUBLIC ${LibMagic_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${LibMagic_LIBRARIES})
    target_compile_definitions(libgerbera PUBLIC HAVE_MAGIC)
endif()

if(WITH_AVCODEC)
    find_package(FFMPEG REQUIRED)
    target_link_libraries(libgerbera PUBLIC PkgConfig::FFMPEG)
    target_compile_definitions(libgerbera PUBLIC HAVE_FFMPEG)

    include(CheckStructHasMember)
    set(CMAKE_REQUIRED_LIBRARIES PkgConfig::FFMPEG)
    check_struct_has_member("struct AVStream" codecpar libavformat/avformat.h HAVE_AVSTREAM_CODECPAR LANGUAGE C)
    if (HAVE_AVSTREAM_CODECPAR)
        target_compile_definitions(libgerbera PUBLIC HAVE_AVSTREAM_CODECPAR)
    endif()
endif()

if(WITH_FFMPEGTHUMBNAILER)
    find_package (FFMpegThumbnailer REQUIRED)
    target_include_directories(libgerbera PUBLIC ${FFMPEGTHUMBNAILER_INCLUDE_DIR})
    target_link_libraries(libgerbera PUBLIC ${FFMPEGTHUMBNAILER_LIBRARIES})
    target_compile_definitions(libgerbera PUBLIC HAVE_FFMPEGTHUMBNAILER)
endif()

if(WITH_EXIF)
    find_package(EXIF REQUIRED)
    target_include_directories(libgerbera PUBLIC ${EXIF_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${EXIF_LIBRARIES})
    target_compile_definitions(libgerbera PUBLIC HAVE_LIBEXIF)
endif()

if(WITH_EXIV2)
    find_package(EXIV2 REQUIRED)
    target_include_directories(libgerbera PUBLIC ${EXIV2_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${EXIV2_LIBRARIES})
    target_compile_definitions(libgerbera PUBLIC HAVE_EXIV2)
    # Exiv still uses std::auto_ptr
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_definitions(libgerbera PUBLIC _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR)
    endif()
endif()

if(WITH_MATROSKA)
    find_package(EBML REQUIRED)

    target_include_directories(libgerbera PUBLIC ${EBML_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${EBML_LIBRARIES})

    find_package(Matroska REQUIRED)
    target_include_directories(libgerbera PUBLIC ${Matroska_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${Matroska_LIBRARIES})

    target_compile_definitions(libgerbera PUBLIC HAVE_MATROSKA)
endif()

if(WITH_LASTFM)
    find_package (LastFMLib REQUIRED)
    target_include_directories(libgerbera PUBLIC ${LASTFMLIB_INCLUDE_DIRS})
    target_link_libraries(libgerbera PUBLIC ${LASTFMLIB_LIBRARIES})
    target_compile_definitions(libgerbera PUBLIC HAVE_LASTFMLIB)
endif()

find_package (ZLIB REQUIRED)
target_link_libraries (libgerbera PUBLIC ZLIB::ZLIB)

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

if(WITH_TESTS)
    message(STATUS "Configuring unit tests")
    enable_testing()
    add_subdirectory(test)
endif()

add_executable(zpipe src/database/create/zpipe.c)
target_link_libraries(zpipe ZLIB::ZLIB)

add_custom_command(OUTPUT sqlite3_create_sql.h
        COMMAND bash ARGS build_headers.sh sqlite $<TARGET_FILE:zpipe> ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/database/create
        DEPENDS
            zpipe
            src/database/create/build_headers.sh
            src/database/create/sqlite3.sql
            src/database/create/sqlite3.sql.tmpl.f
            src/database/create/sqlite3.sql.tmpl.h)
target_sources(libgerbera PRIVATE sqlite3_create_sql.h)

if (WITH_MYSQL)
    add_custom_command(OUTPUT mysql_create_sql.h
            COMMAND bash ARGS build_headers.sh mysql $<TARGET_FILE:zpipe> ${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/database/create
            DEPENDS
                zpipe
                src/database/create/build_headers.sh
                src/database/create/mysql.sql
                src/database/create/mysql.sql.tmpl.f
                src/database/create/mysql.sql.tmpl.h)
    target_sources(libgerbera PRIVATE mysql_create_sql.h)
endif()
target_include_directories(libgerbera PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


INSTALL(TARGETS gerbera DESTINATION bin)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/js DESTINATION share/gerbera)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/web DESTINATION share/gerbera)
INSTALL(FILES
        src/database/create/mysql.sql
        src/database/create/sqlite3.sql
        DESTINATION share/gerbera)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/doc/gerbera.1 DESTINATION share/man/man1)
