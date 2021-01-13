add_executable(testscripting
    main.cc
    mock/duk_helper.h
    mock/duk_helper.cc
    mock/common_script_mock.h
    mock/script_test_fixture.h
    mock/script_test_fixture.cc
    test_common_script.cc
    test_external_m3u_playlist.cc
    test_external_pls_playlist.cc
    test_import_script.cc
    test_import_struct_script.cc
    test_internal_m3u_playlist.cc
    test_internal_pls_playlist.cc
    test_runtime.cc
)

target_link_libraries(testscripting PRIVATE
    libgerbera
    GTest::GTest
)
target_compile_definitions(testscripting PRIVATE SCRIPTS_DIR="${CMAKE_SOURCE_DIR}/scripts")

include(GoogleTest)
gtest_discover_tests(testscripting)

