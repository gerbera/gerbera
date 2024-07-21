add_executable(testcontent
    test_autoscan_list.cc
    test_resolution.cc
)

target_link_libraries(testcontent PRIVATE
    libgerbera
    GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(testcontent DISCOVERY_TIMEOUT 60)
