include_directories (../src)

if (GERBERA_USES_CONAN)
    set(GERBERA_GTEST_LIB CONAN_PKG::gtest)
else()
    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
    set(GERBERA_GTEST_LIB ${GTEST_LIBRARIES} ${GMOCK_BOTH_LIBRARIES})

    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

## Add tests below
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(scripting)
add_subdirectory(util)
