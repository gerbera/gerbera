find_package(GTest REQUIRED)

# CMake can't fix gmock support. https://gitlab.kitware.com/cmake/cmake/-/issues/17365
# So for non-conan build we need to find it separately.
if (NOT "${GTest_LIBRARY_LIST}" MATCHES "gmock")
    find_package(GMock REQUIRED)
    set_property(TARGET GTest::GTest APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${GMOCK_LIBRARIES})

    # cmake 3.23 adds target GTest::gmock_main
    # https://cmake.org/cmake/help/v3.23/module/FindGTest.html
    if (CMAKE_VERSION VERSION_LESS "3.23.0")
        add_library(GMock::gmock_main ALIAS GTest::gmock_main)
    endif()
endif()

## Add tests below
add_subdirectory(config)
add_subdirectory(content)
add_subdirectory(core)
if(WITH_JS)
    add_subdirectory(scripting)
endif()
add_subdirectory(database)
add_subdirectory(util)
