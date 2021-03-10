add_executable(testcontent
    main.cc
    test_autoscan_timed.cc
        content_manager_test.cc)

target_link_libraries(testcontent PRIVATE
    libgerbera
    GTest::GTest
)

target_compile_definitions(testcontent PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

include(GoogleTest)
gtest_discover_tests(testcontent)
