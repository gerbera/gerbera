find_package(GTest REQUIRED)

# CMake prior to 3.23.0 can't find gmock. https://gitlab.kitware.com/cmake/cmake/-/issues/17365
# https://cmake.org/cmake/help/v3.23/module/FindGTest.html
if (CMAKE_VERSION VERSION_LESS "3.23.0" AND NOT TARGET GTest::gmock)
    message("No GMock in GTest")
    find_package(GMock REQUIRED)
    set_property(TARGET GTest::GTest APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${GMOCK_LIBRARIES})
    add_library(GTest::gmock_main ALIAS ${GMOCK_MAIN_LIBRARY})
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
