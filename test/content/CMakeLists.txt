add_executable(testcontent
    main.cc
        test_autoscan_list.cc
)

target_link_libraries(testcontent PRIVATE
    libgerbera
    GTest::GTest
)

include(GoogleTest)
gtest_discover_tests(testcontent DISCOVERY_TIMEOUT 60)
