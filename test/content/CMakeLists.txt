add_executable(testcontent
    main.cc
    test_autoscan_list.cc
    test_resolution.cc
)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testcontent PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testcontent PRIVATE
        libgerbera
        GTest::gmock
    )
endif()

include(GoogleTest)
gtest_discover_tests(testcontent DISCOVERY_TIMEOUT 60)
