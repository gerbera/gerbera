find_package(Threads REQUIRED)

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
)

add_test(NAME testconfig COMMAND testconfig)

add_definitions(-DCMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")
