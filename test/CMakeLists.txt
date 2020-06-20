find_package(GTest REQUIRED)

# CMake can't fix gmock support. https://gitlab.kitware.com/cmake/cmake/-/issues/17365
# So for non-conan build we need to find it separately.
if (NOT "${GTest_LIBRARY_LIST}" MATCHES "gmock")
    find_package(GMock REQUIRED)
    set_property(TARGET GTest::GTest APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${GMOCK_LIBRARIES})
endif()

## Add tests below
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(scripting)
add_subdirectory(util)
