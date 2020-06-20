find_package(Threads REQUIRED)

add_executable(testutil
        main.cc
        test_tools.cc
        test_upnp_headers.cc
)

target_link_libraries(testutil PRIVATE
        libgerbera
        GTest::GTest
)

add_test(NAME testutil COMMAND testutil)
