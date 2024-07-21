include(GoogleTest)
generate_compile_info()

add_executable(testcore
    test_searchhandler.cc
    test_server.cc
    test_upnp_xml.cc
    test_ffmpeg_cache_paths.cc
    test_url_utils.cc
)

target_link_libraries(testcore PRIVATE
    libgerbera
    GTest::gmock_main
)
add_custom_command(TARGET testcore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
        DEPENDS fixtures/*
        COMMENT "Copying Core Fixtures"
)
target_compile_definitions(testcore PRIVATE CMAKE_BINARY_DIR="$<TARGET_FILE_DIR:gerbera>")
target_compile_definitions(testcore PRIVATE COMPILE_INFO="${COMPILE_INFO}")
target_compile_definitions(testcore PRIVATE GIT_BRANCH="${GIT_BRANCH}")
target_compile_definitions(testcore PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

include(GoogleTest)
gtest_discover_tests(testcore DISCOVERY_TIMEOUT 60)
