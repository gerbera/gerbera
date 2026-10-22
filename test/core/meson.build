testcore = executable(
  'testcore',
  'test_searchhandler.cc',
  'test_server.cc',
  'test_upnp_map.cc',
  'test_upnp_xml.cc',
  'test_ffmpeg_cache_paths.cc',
  'test_url_utils.cc',
  cpp_args: '-DCMAKE_BINARY_DIR="@0@"'.format(meson.project_build_root()),
  dependencies: [libgerbera_dep, gtest_dep],
)

test(
  'testcore',
  testcore,
  workdir: meson.current_source_dir(),
)
