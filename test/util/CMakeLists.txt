add_executable(testutil
    test_tools.cc
    test_upnp_clients.cc
    test_upnp_headers.cc
    test_jpeg_res.cc
)
target_link_libraries(testutil PRIVATE
    libgerbera
    GTest::gmock_main
)
add_custom_command(TARGET testutil POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/testdata ${CMAKE_CURRENT_BINARY_DIR}/testdata
        DEPENDS testdata/*
        COMMENT "Copying Util Test Data"
)

include(GoogleTest)
gtest_discover_tests(testutil DISCOVERY_TIMEOUT 60)
