testutil = executable(
  'testutil',
  'test_jpeg_res.cc',
  'test_tools.cc',
  'test_upnp_clients.cc',
  'test_upnp_headers.cc',
  dependencies: [libgerbera_dep, gtest_dep],
)

test(
  'testutil',
  testutil,
  workdir: meson.current_source_dir(),
)
