find_package(GTest QUIET)

if (NOT TARGET GTest::gmock_main)
    # Patch up older CMake GTest module
    if (TARGET GTest::GTest)
        message(STATUS "Found Legacy GTest target")
        add_library(GTest::gtest_main ALIAS GTest::Main)
        add_library(GTest::gmock_main ALIAS GTest::Main)
        ### CMake can't fix gmock support. https://gitlab.kitware.com/cmake/cmake/-/issues/17365
        ### So for non-conan build we need to find it separately.
        if (NOT "${GTest_LIBRARY_LIST}" MATCHES "gmock")
            find_package(GMock REQUIRED)
            set_property(TARGET GTest::gmock_main APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${GMOCK_LIBRARIES})
        endif()
    elseif (DOWNLOAD_DEPS)
        message(STATUS "Downloading GTest")
        include(FetchContent)
        FetchContent_Declare(
                googletest
                URL https://github.com/google/googletest/archive/bf66935e07825318ae519675d73d0f3e313b3ec6.zip
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        # Instead of FetchContent_MakeAvailable(googletest)
        # To avoid installing gtest files
        FetchContent_GetProperties(googletest)
        if(NOT googletest_POPULATED)
            FetchContent_Populate(googletest)
            add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()
        add_library(GTest::gtest_main ALIAS gtest_main)
        add_library(GTest::gmock_main ALIAS gmock_main)
    endif()
endif()

if (NOT TARGET GTest::gmock_main)
    message(FATAL_ERROR "GTest (gmock_main) not found. Check install or enable DOWNLOAD_DEPS to fetch automatically")
else()
    message(STATUS "Looking for GTest - found")
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
