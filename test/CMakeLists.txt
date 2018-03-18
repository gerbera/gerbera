include_directories (../src)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
get_target_property(GERBERA_INTERFACE_LIBRARIES gerbera INTERFACE_LINK_LIBRARIES)

## Add tests below
add_subdirectory(test_dictionary)
add_subdirectory(test_runtime)
add_subdirectory(test_config)