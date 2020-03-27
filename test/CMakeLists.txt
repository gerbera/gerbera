include_directories (../src)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
get_target_property(GERBERA_INTERFACE_LIBRARIES gerbera INTERFACE_LINK_LIBRARIES)

## Add tests below
add_subdirectory(test_config)
add_subdirectory(test_core)
add_subdirectory(test_scripting)
add_subdirectory(test_util)
