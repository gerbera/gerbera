configure_file(
  input: meson.project_source_root() / 'src/database/sqlite3/sqlite3.sql',
  output: 'sqlite3.sql',
  copy: true,
)
configure_file(
  input: meson.project_source_root() / 'src/database/sqlite3/sqlite3-drop.sql',
  output: 'sqlite3-drop.sql',
  copy: true,
)
configure_file(
  input: meson.project_source_root() / 'src/database/sqlite3/sqlite3-upgrade.xml',
  output: 'sqlite3-upgrade.xml',
  copy: true,
)
configure_file(
  input: meson.project_source_root() / 'src/database/mysql/mysql.sql',
  output: 'mysql.sql',
  copy: true,
)
configure_file(
  input: meson.project_source_root() / 'src/database/mysql/mysql-drop.sql',
  output: 'mysql-drop.sql',
  copy: true,
)
configure_file(
  input: meson.project_source_root() / 'src/database/mysql/mysql-upgrade.xml',
  output: 'mysql-upgrade.xml',
  copy: true,
)

testdatabase = executable(
  'testdatabase',
  'test_database.cc',
  'test_sql_generators.cc',
  dependencies: [libgerbera_dep, gtest_dep],
)

test(
  'testdatabase',
  testdatabase,
  workdir: meson.current_build_dir(),
)
