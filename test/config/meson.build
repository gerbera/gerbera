testconfig = executable(
  'testconfig',
  'test_configgenerator.cc',
  'test_configmanager.cc',
  cpp_args: ['-DGERBERA_TEST="gerbera-test"', '-DCMAKE_BINARY_DIR="@0@"'.format(meson.project_source_root())],
  dependencies: [libgerbera_dep, gtest_dep],
)

test(
  'testconfig',
  testconfig,
  workdir: meson.current_source_dir(),
)
