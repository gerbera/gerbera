find_package(Threads REQUIRED)

add_executable(testcore
        main.cc
        test_searchhandler.cc
        test_server.cc
        test_upnp_xml.cc
        test_ffmpeg_cache_paths.cc
)

target_link_libraries(testcore PRIVATE
        libgerbera
        GTest::GTest
        )

add_test(NAME testcore COMMAND testcore)

add_custom_command(TARGET testcore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
        )

target_compile_definitions(testcore PRIVATE -DCMAKE_BINARY_DIR="$<TARGET_FILE_DIR:gerbera>")
