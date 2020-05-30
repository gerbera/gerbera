find_package(Threads REQUIRED)

add_executable(testutil
        test_tools.cc
        test_upnp_headers.cc
)

target_link_libraries(testutil PRIVATE
        libgerbera
        ${GERBERA_GTEST_LIB}
)

add_test(NAME testutil COMMAND testutil)
