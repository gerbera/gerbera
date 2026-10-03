if not duktape_dep.found() or not curl_dep.found()
  subdir_done()
endif

testscripting = executable(
  'testscripting',
  'mock/duk_helper.cc',
  'mock/script_test_fixture.cc',
  'test_common_script.cc',
  'test_external_asx_playlist.cc',
  'test_external_m3u_playlist.cc',
  'test_external_pls_playlist.cc',
  'test_import_script.cc',
  'test_import_struct_script.cc',
  'test_internal_m3u_playlist.cc',
  'test_internal_m3u8_playlist.cc',
  'test_internal_pls_playlist.cc',
  'test_runtime.cc',
  cpp_args: '-DSCRIPTS_DIR="@0@/scripts"'.format(meson.project_source_root()),
  dependencies: [libgerbera_dep, gtest_dep],
)

test(
  'testscripting',
  testscripting,
  workdir: meson.current_source_dir(),
)
