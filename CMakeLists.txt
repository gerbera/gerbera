cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
cmake_policy(VERSION 3.13)

# Only set for release tags
# You should only see version "git" when built from a git snapshot tarball
set(GERBERA_VERSION "git")

project(Gerbera
        DESCRIPTION "UPnP Media Server"
        HOMEPAGE_URL "https://gerbera.io"
        LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# If Conan stuff is present, load it
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL)
if(COMMAND conan_basic_setup)
    # Prefer the conan FindModules
    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
    conan_basic_setup(TARGETS)
endif()

set(WITH_NPUPNP            NO  CACHE BOOL "Use npupnp instead of libupnp")
set(WITH_MAGIC             YES CACHE BOOL "Use libmagic to identify file mime types")
set(WITH_MYSQL             NO  CACHE BOOL "Store media information in MySQL DB")
set(WITH_CURL              YES CACHE BOOL "CURL required for online services")
set(WITH_INOTIFY           YES CACHE BOOL "Enable Inotify file monitoring support")
set(WITH_JS                YES CACHE BOOL "Enable JavaScript for scripting")
set(WITH_TAGLIB            YES CACHE BOOL "Use TagLib to extract audio file metadata")
set(WITH_AVCODEC           NO  CACHE BOOL "Use ffmpeg/libav to extract file metadata")
set(WITH_FFMPEGTHUMBNAILER NO  CACHE BOOL "Enable Thumbnail generation")
set(WITH_EXIF              YES CACHE BOOL "Use libexif to extract image metadata")
set(WITH_EXIV2             NO  CACHE BOOL "Use libexiv2 to extract image metadata")
set(WITH_MATROSKA          YES CACHE BOOL "Use libmatroska to extract video/mkv metadata")
set(WITH_SYSTEMD           YES CACHE BOOL "Install Systemd unit file")
set(WITH_LASTFM            NO  CACHE BOOL "Enable scrobbling to LastFM")
set(WITH_DEBUG             YES CACHE BOOL "Enables debug logging")
set(WITH_TESTS             NO  CACHE BOOL "Build unit tests")

# For building packages without depending on the old system libupnp
set(STATIC_LIBUPNP 0 CACHE BOOL "Link to libupnp statically")

# Leverage a modern C++
# Set defaults here before we create targets
set(CMAKE_CXX_STANDARD 17)
# Cygwin requires extensions for a lot of the networking stuff.
if(CYGWIN)
	set(CMAKE_CXX_STANDARD_REQUIRED NO)
	set(CMAKE_CXX_EXTENSIONS YES)
else(CYGWIN)
	set(CMAKE_CXX_STANDARD_REQUIRED YES)
	set(CMAKE_CXX_EXTENSIONS NO)
endif(CYGWIN)

add_library(libgerbera STATIC
        src/action_request.cc
        src/action_request.h
        src/cds_objects.cc
        src/cds_objects.h
        src/cds_resource.cc
        src/cds_resource.h
        src/common.h
        src/config/client_config.h
        src/config/client_config.cc
        src/config/config_definition.h
        src/config/config_definition.cc
        src/config/config_generator.h
        src/config/config_generator.cc
        src/config/config_manager.cc
        src/config/config_manager.h
        src/config/config_options.cc
        src/config/config_options.h
        src/config/config_setup.cc
        src/config/config_setup.h
        src/config/directory_tweak.cc
        src/config/directory_tweak.h
        src/config/dynamic_content.cc
        src/config/dynamic_content.h
        src/content/autoscan.cc
        src/content/autoscan.h
        src/content/autoscan_list.cc
        src/content/autoscan_list.h
        src/content/autoscan_inotify.cc
        src/content/autoscan_inotify.h
        src/content/content_manager.cc
        src/content/content_manager.h
        src/content/layout/builtin_layout.cc
        src/content/layout/builtin_layout.h
        src/content/layout/js_layout.cc
        src/content/layout/js_layout.h
        src/content/layout/layout.h
        src/content/onlineservice/atrailers_content_handler.cc
        src/content/onlineservice/atrailers_content_handler.h
        src/content/onlineservice/atrailers_service.cc
        src/content/onlineservice/atrailers_service.h
        src/content/onlineservice/curl_online_service.cc
        src/content/onlineservice/curl_online_service.h
        src/content/onlineservice/lastfm_scrobbler.cc
        src/content/onlineservice/lastfm_scrobbler.h
        src/content/onlineservice/online_service.cc
        src/content/onlineservice/online_service.h
        src/content/onlineservice/online_service_helper.cc
        src/content/onlineservice/online_service_helper.h
        src/content/onlineservice/task_processor.cc
        src/content/onlineservice/task_processor.h
        src/content/scripting/import_script.cc
        src/content/scripting/import_script.h
        src/content/scripting/js_functions.cc
        src/content/scripting/js_functions.h
        src/content/scripting/playlist_parser_script.cc
        src/content/scripting/playlist_parser_script.h
        src/content/scripting/scripting_runtime.cc
        src/content/scripting/scripting_runtime.h
        src/content/scripting/script.cc
        src/content/scripting/script.h
        src/content/scripting/script_names.h
        src/content/update_manager.cc
        src/content/update_manager.h
        src/context.cc
        src/context.h
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
        src/request_handler.cc
        src/request_handler.h
        src/server.cc
        src/server.h
        src/database/mysql/mysql_database.cc
        src/database/mysql/mysql_database.h
        src/database/sqlite3/sqlite_database.cc
        src/database/sqlite3/sqlite_database.h
        src/database/sql_database.cc
        src/database/sql_database.h
        src/database/sql_format.h
        src/database/database.cc
        src/database/database.h
        src/database/search_handler.cc
        src/database/search_handler.h
        src/subscription_request.cc
        src/subscription_request.h
        src/transcoding/transcode_dispatcher.cc
        src/transcoding/transcode_dispatcher.h
        src/transcoding/transcode_ext_handler.cc
        src/transcoding/transcode_ext_handler.h
        src/transcoding/transcode_handler.cc
        src/transcoding/transcode_handler.h
        src/transcoding/transcoding.cc
        src/transcoding/transcoding.h
        src/upnp_cds.cc
        src/upnp_cds.h
        src/upnp_cm.cc
        src/upnp_cm.h
        src/upnp_mrreg.cc
        src/upnp_mrreg.h
        src/upnp_xml.cc
        src/upnp_xml.h
        src/url_request_handler.cc
        src/url_request_handler.h
        src/util/executor.h
        src/util/generic_task.cc
        src/util/generic_task.h
        src/util/grb_fs.cc
        src/util/grb_fs.h
        src/util/jpeg_resolution.cc
        src/util/logger.h
        src/util/mime.cc
        src/util/mime.h
        src/util/mt_inotify.cc
        src/util/mt_inotify.h
        src/util/process_executor.cc
        src/util/process_executor.h
        src/util/string_converter.cc
        src/util/string_converter.h
        src/util/thread_executor.cc
        src/util/thread_executor.h
        src/util/thread_runner.h
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
        src/util/url.cc
        src/util/url.h
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
target_link_libraries(gerbera PRIVATE libgerbera)

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

# Generate Compile Information
include("GenCompileInfo")
generate_compile_info()
target_compile_definitions(gerbera PRIVATE COMPILE_INFO="${COMPILE_INFO}")
target_compile_definitions(gerbera PRIVATE GIT_BRANCH="${GIT_BRANCH}")
target_compile_definitions(gerbera PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

if ((${GERBERA_VERSION} STREQUAL "git") AND (NOT "${GIT_TAG}" STREQUAL ""))
    # Master/git build so use git info
    set(GERBERA_VERSION ${GIT_TAG})
endif()
message(STATUS "Gerbera Version: ${GERBERA_VERSION}")
target_compile_definitions(libgerbera PUBLIC GERBERA_VERSION="${GERBERA_VERSION}")

target_compile_definitions(gerbera PRIVATE PACKAGE_DATADIR="${CMAKE_INSTALL_PREFIX}/share/gerbera")

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
    pkg_check_modules(NPUPNP QUIET libnpupnp>=4.1.2)
    if (NOT NPUPNP_FOUND)
        find_package(NPUPNP REQUIRED)
    endif()
    target_include_directories(libgerbera PUBLIC ${NPUPNP_INCLUDE_DIRS})
    target_compile_definitions(libgerbera PUBLIC USING_NPUPNP)
    target_link_libraries(libgerbera PUBLIC ${NPUPNP_LIBRARIES})
else()
    set(REQ_UPNP_VERSION 1.14.0)
    # LibUPnP official target since 1.16 (Lib version 18)
    # This will prefer the provided UPNPConfig.cmake if found, if not, it will fall back our FindUPNP.cmake
    find_package(UPNP ${REQ_UPNP_VERSION} QUIET)
    if(NOT TARGET UPNP::UPNP)
        # Conan packages have autogenerated CMake packages under pupnp::pupnp
        find_package(pupnp ${REQ_UPNP_VERSION} QUIET)
        if(TARGET pupnp::pupnp)#
            add_library(UPNP::UPNP ALIAS pupnp::pupnp)
        endif()
    endif()
    if(NOT TARGET UPNP::UPNP)
        message(FATAL_ERROR "Gerbera failed to find libupnp! at least version ${REQ_UPNP_VERSION} is needed.")
    endif()
    # Conan autogenerated targets dont set a version, but to set name_VERSION var
    get_target_property(_UPNP_VERSION UPNP::UPNP VERSION)
    if (NOT _UPNP_VERSION)
        set(_UPNP_VERSION ${pupnp_VERSION})
    endif()
    get_target_property(_UPNP_LIB UPNP::UPNP LOCATION)
    # Provide our own status message as we search QUIET-ly
    message(STATUS "Found libupnp: ${_UPNP_LIB} (found version ${_UPNP_VERSION})")

    # Check and warn for libupnp features
    include(CheckCXXSymbolExists)
    get_target_property(UPNP_INCLUDE_DIR UPNP::UPNP INTERFACE_INCLUDE_DIRECTORIES)
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

    check_cxx_symbol_exists(UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS "${UPNP_INCLUDE_DIR}/upnpconfig.h" UPNP_HAS_BLOCKING_TCP)
    if (UPNP_HAS_BLOCKING_TCP)
        message(WARNING [=[
    !! It is strongly recommended to build libupnp with --disable-blocking-tcp-connections !!
    Without this option non-responsive control points can cause libupnp to hang.]=])
    endif()

    check_cxx_symbol_exists(UPNP_HAVE_TOOLS "${UPNP_INCLUDE_DIR}/upnpconfig.h" UPNP_HAS_TOOLS)
    if (NOT UPNP_HAS_TOOLS)
        message(WARNING [=[
    !! It is strongly recommended to build libupnp with --enable-tools !!
        ]=])
    endif()

    target_link_libraries(libgerbera PUBLIC UPNP::UPNP)
endif()

find_package(fmt 7.1.3 REQUIRED)
target_link_libraries(libgerbera PUBLIC fmt::fmt)

find_package(spdlog 1.8.1 REQUIRED)
target_link_libraries(libgerbera PUBLIC spdlog::spdlog)

find_package(pugixml REQUIRED)
target_link_libraries(libgerbera PUBLIC pugixml::pugixml)

find_package(SQLite3 3.7.11 REQUIRED)
target_link_libraries(libgerbera PUBLIC SQLite::SQLite3)

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
    find_package(Inotify REQUIRED)
    target_link_libraries(libgerbera PUBLIC Inotify::Inotify)
    target_compile_definitions(libgerbera PUBLIC HAVE_INOTIFY)
endif()

if(WITH_JS)
    find_package(duktape REQUIRED)
    target_link_libraries(libgerbera PUBLIC duktape::duktape)
    target_compile_definitions(libgerbera PUBLIC HAVE_JS)
endif()

if(WITH_MYSQL)
    find_package(mariadb-connector-c QUIET)
    if(mariadb-connector-c_FOUND)
        target_include_directories(libgerbera PUBLIC ${mariadb-connector-c_INCLUDE_DIRS})
        target_link_libraries(libgerbera PUBLIC ${mariadb-connector-c_LIBS})
        target_compile_definitions(libgerbera PUBLIC HAVE_MYSQL)

        find_package(OpenSSL REQUIRED)
        target_include_directories(libgerbera PUBLIC ${OpenSSL_INCLUDE_DIRS})
        target_link_libraries(libgerbera PUBLIC ${OpenSSL_LIBS})
    else()
        find_package(MySQL REQUIRED)
        target_include_directories(libgerbera PUBLIC ${MYSQL_INCLUDE_DIRS})
        target_link_libraries(libgerbera PUBLIC ${MYSQL_CLIENT_LIBS})
        target_compile_definitions(libgerbera PUBLIC HAVE_MYSQL)
    endif()
endif()

if(WITH_CURL)
    find_package(CURL REQUIRED)
    target_link_libraries(libgerbera PUBLIC CURL::libcurl)

    target_compile_definitions(libgerbera PUBLIC
        HAVE_CURL
        ONLINE_SERVICES
        ATRAILERS)
endif()

if(WITH_TAGLIB)
    find_package(Taglib 1.12 REQUIRED)
    target_link_libraries(libgerbera PUBLIC Taglib::Taglib)
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
    find_package(LibExiv2 REQUIRED)
    target_link_libraries(libgerbera PUBLIC LibExiv2::LibExiv2)
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
            set (SYSTEMD_AFTER_TARGET "mysql.target network-online.target")
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

INSTALL(TARGETS gerbera DESTINATION bin)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/js DESTINATION share/gerbera)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/web DESTINATION share/gerbera)
INSTALL(FILES
        src/database/mysql/mysql.sql
        src/database/mysql/mysql-upgrade.xml
        src/database/sqlite3/sqlite3.sql
        src/database/sqlite3/sqlite3-upgrade.xml
        DESTINATION share/gerbera)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/doc/gerbera.1 DESTINATION share/man/man1)

include(packaging)
