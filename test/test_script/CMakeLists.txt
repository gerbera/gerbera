find_package(Threads REQUIRED)

add_executable(testscript
        $<TARGET_OBJECTS:libgerbera>
        main.cc
        mock/duk_helper.h
        mock/duk_helper.cc
        mock/common_script_mock.h
        mock/script_test_fixture.h
        mock/script_test_fixture.cc
        test_common_script.cc
        test_external_m3u_playlist.cc
        test_external_pls_playlist.cc
        test_import_script.cc
        test_import_struct_script.cc
        test_internal_m3u_playlist.cc
        test_internal_pls_playlist.cc)

include(DefFileName)
define_file_path_for_sources(testscript)

include_directories(
        ${UPNP_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
        ${MAGIC_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${LASTFMLIB_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIR}
        ${EXIF_INCLUDE_DIRS}
        ${TAGLIB_INCLUDE_DIRS}
        ${EXPAT_INCLUDE_DIRS}
        ${FFMPEGTHUMBNAILER_INCLUDE_DIR}
        ${DUKTAPE_INCLUDE_DIRS}
        ${MYSQL_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
        ${ICONV_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}
        ${GMOCK_INCLUDE_DIRS}
)

target_link_libraries(testscript PRIVATE
        ${UUID_LIBRARIES}
        ${UPNP_LIBRARIES}
        ${MAGIC_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${CURL_LIBRARIES}
        ${LASTFMLIB_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${EXIF_LIBRARIES}
        ${TAGLIB_LIBRARIES}
        ${EXPAT_LIBRARIES}
        ${FFMPEGTHUMBNAILER_LIBRARIES}
        ${DUKTAPE_LIBRARIES}
        ${MYSQL_CLIENT_LIBS}
        ${SQLITE3_LIBRARIES}
        ${ICONV_LIBRARIES}
        ${GTEST_LIBRARIES}
        ${GMOCK_BOTH_LIBRARIES}
        ${GERBERA_INTERFACE_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

add_test(NAME testscript
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/test_script
        COMMAND ./testscript)

add_definitions(-DSCRIPTS_DIR="${CMAKE_SOURCE_DIR}/scripts")