libgerbera_args += '-DSCRIPTS_DIR="@0@/scripts"'.format(meson.project_source_root())

testscripting = executable(
  'testscripting',
  'main.cc',
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
  cpp_args : libgerbera_args,
  dependencies : libgerbera_deps,
  include_directories : libgerbera_incdir,
  link_with : libgerbera,
)

test('testscripting', testscripting, workdir : meson.current_source_dir())
