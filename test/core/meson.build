libgerbera_args += '-UCMAKE_BINARY_DIR'
libgerbera_args += '-DCMAKE_BINARY_DIR="@0@"'.format(meson.project_build_root())

testcore = executable(
  'testcore',
  'main.cc',
  'test_searchhandler.cc',
  'test_server.cc',
  'test_upnp_xml.cc',
  'test_ffmpeg_cache_paths.cc',
  'test_url_utils.cc',
  cpp_args : libgerbera_args,
  dependencies : libgerbera_deps,
  include_directories : libgerbera_incdir,
  link_with : libgerbera,
)

test('testcore', testcore, workdir : meson.current_source_dir())
