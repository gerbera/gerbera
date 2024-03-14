add_executable(testconfig
    main.cc
    test_configgenerator.cc
    test_configmanager.cc
)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testconfig PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testconfig PRIVATE
        libgerbera
        GTest::gmock
    )
endif()

add_custom_command(TARGET testconfig POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
    DEPENDS fixtures/*
    COMMENT "Copying Config Fixtures"
)
target_compile_definitions(testconfig PRIVATE CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")
add_dependencies(testconfig gerbera)

include(GoogleTest)
gtest_discover_tests(testconfig DISCOVERY_TIMEOUT 60)
