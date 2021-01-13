add_executable(testcontent
    main.cc
    test_autoscan_timed.cc
)

target_link_libraries(testcontent PRIVATE
    libgerbera
    GTest::GTest
)

include(GoogleTest)
gtest_discover_tests(testcontent)
