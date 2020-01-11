find_package(Threads REQUIRED)

add_executable(testconfig
        $<TARGET_OBJECTS:libgerbera>
        main.cc
        test_configgenerator.cc
        test_configmanager.cc
        )

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
)

target_link_libraries(testconfig PRIVATE
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
        ${GERBERA_INTERFACE_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

add_custom_command(TARGET testconfig POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures $<TARGET_FILE_DIR:testconfig>/fixtures
)

add_test(NAME testconfig
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/test_config
        COMMAND ./testconfig)

add_definitions(-DCMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")
