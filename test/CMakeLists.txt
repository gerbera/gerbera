include_directories (../src)

message(STATUS "Configuring unit tests")
enable_testing()

if (GERBERA_USES_CONAN)
    set(GERBERA_GTEST_LIB CONAN_PKG::gtest)
else()
    set(GERBERA_GTEST_LIB ${GMOCK_BOTH_LIBRARIES})

    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

get_target_property(GERBERA_INTERFACE_LIBRARIES gerbera INTERFACE_LINK_LIBRARIES)

## Add tests below
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(scripting)
add_subdirectory(util)
