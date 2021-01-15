add_executable(testconfig
    main.cc
    test_configgenerator.cc
    test_configmanager.cc
)

target_link_libraries(testconfig PRIVATE
    libgerbera
    GTest::GTest
)

add_custom_command(TARGET testconfig POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
    DEPENDS fixtures/*
    COMMENT "Copying Fixtures"
)
target_compile_definitions(testconfig PRIVATE CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")
add_dependencies(testconfig gerbera)

include(GoogleTest)
gtest_discover_tests(testconfig)
