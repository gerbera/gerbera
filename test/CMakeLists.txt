include("RetrieveGoogleTest")
include_directories (../src)
find_package( Boost REQUIRED COMPONENTS filesystem )

## Add tests below
add_subdirectory(test_dictionary)
add_subdirectory(test_runtime)