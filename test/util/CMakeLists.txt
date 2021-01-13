add_executable(testutil
    main.cc
    test_tools.cc
    test_upnp_clients.cc
    test_upnp_headers.cc
)
target_link_libraries(testutil PRIVATE
    libgerbera
    GTest::GTest
)

include(GoogleTest)
gtest_discover_tests(testutil)
