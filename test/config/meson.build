libgerbera_args += '-DCMAKE_BINARY_DIR="@0@"'.format(meson.project_source_root())

testconfig = executable(
  'testconfig',
  'main.cc',
  'test_configgenerator.cc',
  'test_configmanager.cc',
  cpp_args : [libgerbera_args, '-DGERBERA_TEST="gerbera-test"'],
  dependencies : libgerbera_deps,
  include_directories : libgerbera_incdir,
  link_with : libgerbera,
)

test('testconfig', testconfig, workdir : meson.current_source_dir())
