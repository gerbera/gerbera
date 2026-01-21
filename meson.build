project(
  'gerbera',
  'c',
  'cpp',
  version: '1.11.0',
  meson_version: '>=0.60',
  default_options: [
    'cpp_std=c++17',
    'warning_level=1',
    'default_library=static',
  ],
)

cpp = meson.get_compiler('cpp')
if cpp.get_id() == 'gcc' and cpp.version().version_compare('<8.4')
  error('Your GCC version is too old. You need at least version 8.4.')
elif cpp.get_id() == 'clang' and cpp.version().version_compare('<7')
  error('Your clang version is too old. You need at least version 7.')
endif

# This makes assumptions that the libcpp is the GNU one on Linux systems
if cpp.version().version_compare('>=9')
  fs_dep = declare_dependency()
elif host_machine.system() == 'linux'
  fs_dep = cpp.find_library('stdc++fs')
elif cpp.get_id() == 'clang'
  fs_dep = cpp.find_library('c++fs')
endif

jsoncpp_dep = dependency('jsoncpp')
pugixml_dep = dependency('pugixml')
fmt_dep = dependency(
  'fmt',
  version: '>=7.1.3',
)
spdlog_dep = dependency(
  'spdlog',
  version: '>=1.8.1',
)
sqlite3_dep = dependency(
  'sqlite3',
  version: '>=3.7.11',
)

if cpp.links('#include <atomic>\nint main(){std::atomic<long long> b;return b.fetch_add(5);}')
  libatomic_dep = declare_dependency()
else
  libatomic_dep = cpp.find_library('atomic')
endif

libgerbera_args = [
  '-DGERBERA_VERSION="@0@"'.format(meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(get_option('prefix') / get_option('datadir'), meson.project_name()),
]

if (cpp.has_function('nl_langinfo'))
  libgerbera_args += '-DHAVE_NL_LANGINFO'
endif

if (cpp.has_function('setlocale'))
  libgerbera_args += '-DHAVE_SETLOCALE'
endif

if host_machine.system() == 'freebsd'
  uuid_dep = declare_dependency()
  libgerbera_args += '-DBSD_NATIVE_UUID'
else
  uuid_dep = dependency('uuid')
endif

if host_machine.system() == 'sunos'
  socket_dep = cpp.find_library('socket')
else
  socket_dep = []
endif

if host_machine.system() == 'sunos'
  libgerbera_args += '-DSOLARIS'
elif host_machine.system() != 'linux'
  libgerbera_args += '-DBSD'
endif

if get_option('debug')
  libgerbera_args += '-DTOMBDEBUG'
  libgerbera_args += '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG'
else
  libgerbera_args += '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO'
endif

avcodec_dep = dependency(
  'libavcodec',
  required: get_option('avcodec'),
)
avfilter_dep = dependency(
  'libavfilter',
  required: get_option('avcodec'),
)
avformat_dep = dependency(
  'libavformat',
  required: get_option('avcodec'),
)
avutil_dep = dependency(
  'libavutil',
  required: get_option('avcodec'),
)
if avcodec_dep.found() and avfilter_dep.found() and avformat_dep.found() and avutil_dep.found()
  libgerbera_args += '-DHAVE_FFMPEG'
  if cpp.has_member(
    'struct AVStream',
    'codecpar',
    prefix: '#include <libavformat/avformat.h>',
    dependencies: avformat_dep,
  )
    libgerbera_args += '-DHAVE_AVSTREAM_CODECPAR'
  endif
endif

curl_dep = dependency(
  'libcurl',
  required: get_option('curl'),
)
if curl_dep.found()
  libgerbera_args += '-DHAVE_CURL'
  libgerbera_args += '-DONLINE_SERVICES'
endif

#hack for FreeBSD which includes external libiconv for libcurl
if host_machine.system() == 'freebsd' and curl_dep.found()
  iconv_dep = cpp.find_library('iconv', dirs: '/usr/local/lib')
else
  iconv_dep = dependency('iconv')
endif

if cpp.compiles('#include <iconv.h\nsize_t iconv(iconv_t,const char**,size_t*,char**,size_t*);')
  libgerbera_args += '-DICONV_CONST'
endif

exif_dep = dependency(
  'libexif',
  required: get_option('exif'),
)
if exif_dep.found()
  libgerbera_args += '-DHAVE_LIBEXIF'
  libgerbera_args += '-DEXIF_VERSION="@0@"'.format(exif_dep.version())
endif

exiv2_dep = dependency(
  'exiv2',
  required: get_option('exiv2'),
)
if exiv2_dep.found()
  libgerbera_args += '-DHAVE_EXIV2'
  if exiv2_dep.version().version_compare('<0.28')
    libgerbera_args += '-D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR'
  endif
endif

ffmpegthumbnailer_dep = dependency(
  'libffmpegthumbnailer',
  required: get_option('ffmpegthumbnailer'),
)
if ffmpegthumbnailer_dep.found()
  libgerbera_args += '-DHAVE_FFMPEGTHUMBNAILER'
  libgerbera_args += '-DFFMPEGTHUMBNAILER_VERSION="@0@"'.format(
    ffmpegthumbnailer_dep.version(),
  )
endif

if host_machine.system() == 'linux' or host_machine.system() == 'sunos'
  inotify_dep = declare_dependency()
  inotify_found = not get_option('inotify').disabled()
else
  inotify_dep = dependency(
    'inotify',
    required: get_option('inotify'),
  )
  inotify_found = inotify_dep.found()
endif

if inotify_found
  libgerbera_args += '-DHAVE_INOTIFY'
endif

duktape_dep = dependency(
  'duktape',
  required: get_option('js'),
)
if duktape_dep.found()
  libgerbera_args += '-DHAVE_JS'
endif

lastfm_dep = dependency(
  'liblastfm',
  required: get_option('lastfm'),
)
if lastfm_dep.found()
  libgerbera_args += '-DHAVE_LASTFMLIB'
  libgerbera_args += '-DHAVE_LASTFM'
elif curl_dep.found()
  libgerbera_args += '-DHAVE_LASTFM'
endif

magic_dep = cpp.find_library(
  'magic',
  required: get_option('magic'),
)
if magic_dep.found()
  libgerbera_args += '-DHAVE_MAGIC'
endif

ebml_dep = dependency(
  'libebml',
  required: get_option('matroska'),
)
matroska_dep = dependency(
  'libmatroska',
  required: get_option('matroska'),
)
if ebml_dep.found() and matroska_dep.found()
  libgerbera_args += '-DHAVE_MATROSKA'
endif

mysql_dep = dependency(
  'libmariadb',
  required: get_option('mysql'),
)
if mysql_dep.found()
  libgerbera_args += '-DHAVE_MYSQL'
endif

pqxx_dep = dependency(
  'libpqxx',
  required: get_option('pgsql'),
)
if pqxx_dep.found()
  libgerbera_args += '-DHAVE_PGSQL'
endif

upnp_dep = dependency(
  'libnpupnp',
  version: '>= 4.2.1',
  required: get_option('npupnp'),
)
if upnp_dep.found()
  libgerbera_args += '-DUSING_NPUPNP'
else
  upnp_dep = dependency(
    'libupnp',
    version: '>= 1.14.6',
  )
endif

taglib_dep = dependency(
  'taglib',
  version: '>= 1.12',
  required: get_option('taglib'),
)
if taglib_dep.found()
  libgerbera_args += '-DHAVE_TAGLIB'
endif

wavpack_dep = dependency(
  'wavpack',
  required: get_option('wavpack'),
)
if wavpack_dep.found()
  libgerbera_args += '-DHAVE_WAVPACK'
endif

gtest_dep = dependency(
  'gmock_main',
  required: get_option('tests'),
)

systemd = dependency(
  'systemd',
  required: get_option('systemd'),
)
if systemd.found()
  systemd_conf = configuration_data()
  systemd_conf.set('CMAKE_INSTALL_PREFIX', get_option('prefix'))
  if mysql_dep.found()
    systemd_conf.set('SYSTEMD_DESCRIPTION', 'Gerbera Media Server and MySQL')
    systemd_conf.set(
      'SYSTEMD_AFTER_TARGET',
      'mysql.target network-online.target',
    )
  elif pqxx_dep.found()
    systemd_conf.set('SYSTEMD_DESCRIPTION', 'Gerbera Media Server and PostgreSQL')
    systemd_conf.set(
      'SYSTEMD_AFTER_TARGET',
      'postgresql.target network-online.target',
    )
  else
    systemd_conf.set('SYSTEMD_DESCRIPTION', 'Gerbera Media Server')
    systemd_conf.set(
      'SYSTEMD_AFTER_TARGET',
      'network.target network-online.target',
    )
  endif
  systemd_conf.set('SYSTEMD_WANTS_TARGET', 'network-online.target')

  configure_file(
    input: 'scripts/systemd/gerbera.service.cmake',
    output: 'gerbera.service',
    configuration: systemd_conf,
    format: 'cmake',
    install_dir: get_option('prefix') / 'lib/systemd/user/',
  )
endif

libgerbera_deps = [
  avcodec_dep,
  avfilter_dep,
  avformat_dep,
  avutil_dep,
  curl_dep,
  duktape_dep,
  ebml_dep,
  exif_dep,
  exiv2_dep,
  fs_dep,
  fmt_dep,
  ffmpegthumbnailer_dep,
  iconv_dep,
  inotify_dep,
  jsoncpp_dep,
  lastfm_dep,
  libatomic_dep,
  magic_dep,
  matroska_dep,
  mysql_dep,
  pqxx_dep,
  pugixml_dep,
  socket_dep,
  spdlog_dep,
  sqlite3_dep,
  taglib_dep,
  upnp_dep,
  uuid_dep,
  wavpack_dep,
]

compile_info = [
  'WITH_INOTIFY=' + inotify_found.to_string('YES', 'NO'),
  'WITH_DEBUG=' + get_option('debug').to_string('YES', 'NO'),
]

cinfos = {
  'MAGIC': magic_dep,
  'MYSQL': mysql_dep,
  'PGSQL': pqxx_dep,
  'CURL': curl_dep,
  'JS': duktape_dep,
  'TAGLIB': taglib_dep,
  'AVCODEC': avcodec_dep,
  'FFMPEGTHUMBNAILER': ffmpegthumbnailer_dep,
  'EXIF': exif_dep,
  'EXIV2': exiv2_dep,
  'SYSTEMD': systemd,
  'LASTFM': lastfm_dep,
  'TESTS': gtest_dep,
}
foreach w, d : cinfos
  compile_info += 'WITH_@0@=@1@'.format(w, d.found().to_string('YES', 'NO'))
endforeach

add_project_arguments(
  '-DCOMPILE_INFO="@0@"'.format('\\n'.join(compile_info)),
  language: 'cpp',
)

g = find_program(
  'git',
  required: false,
)
if g.found()
  gchash = run_command(
    'git',
    'rev-parse',
    'HEAD',
    check: false,
    capture: true,
  )
  if gchash.returncode() == 0
    add_project_arguments(
      '-DGIT_COMMIT_HASH="@0@"'.format(gchash.stdout().strip()),
      language: 'cpp',
    )
  else
    add_project_arguments(
      '-DGIT_COMMIT_HASH=""',
      language: 'cpp',
    )
  endif
  gbranch = run_command(
    'git',
    'rev-parse',
    '--symbolic-full-name',
    'HEAD',
    check: false,
    capture: true,
  )
  if gbranch.returncode() == 0
    add_project_arguments(
      '-DGIT_BRANCH="@0@"'.format(gbranch.stdout().strip()),
      language: 'cpp',
    )
  else
    add_project_arguments(
      '-DGIT_BRANCH=""',
      language: 'cpp',
    )
  endif
else
  add_project_arguments(
    '-DGIT_BRANCH=""',
    language: 'cpp',
  )
  add_project_arguments(
    '-DGIT_COMMIT_HASH=""',
    language: 'cpp',
  )
endif

libgerbera_incdir = include_directories('src')

libgerbera_files = files(
  'src/action_request.cc',
  'src/cds/cds_container.cc',
  'src/cds/cds_enums.cc',
  'src/cds/cds_item.cc',
  'src/cds/cds_objects.cc',
  'src/cds/cds_resource.cc',
  'src/config/config_definition.cc',
  'src/config/config_generator.cc',
  'src/config/config_manager.cc',
  'src/config/config_options.cc',
  'src/config/config_setup.cc',
  'src/config/grb_compile_info.cc',
  'src/config/grb_runtime.cc',
  'src/config/result/autoscan.cc',
  'src/config/result/box_layout.cc',
  'src/config/result/client_config.cc',
  'src/config/result/directory_tweak.cc',
  'src/config/result/dynamic_content.cc',
  'src/config/result/transcoding.cc',
  'src/config/setup/config_setup_array.cc',
  'src/config/setup/config_setup_autoscan.cc',
  'src/config/setup/config_setup_bool.cc',
  'src/config/setup/config_setup_boxlayout.cc',
  'src/config/setup/config_setup_client.cc',
  'src/config/setup/config_setup_dictionary.cc',
  'src/config/setup/config_setup_dynamic.cc',
  'src/config/setup/config_setup_int.cc',
  'src/config/setup/config_setup_path.cc',
  'src/config/setup/config_setup_string.cc',
  'src/config/setup/config_setup_time.cc',
  'src/config/setup/config_setup_transcoding.cc',
  'src/config/setup/config_setup_tweak.cc',
  'src/config/setup/config_setup_vector.cc',
  'src/content/autoscan_list.cc',
  'src/content/autoscan_setting.cc',
  'src/content/cm_task.cc',
  'src/content/content_manager.cc',
  'src/content/import_service.cc',
  'src/content/inotify/autoscan_inotify.cc',
  'src/content/inotify/directory_watch.cc',
  'src/content/inotify/inotify_handler.cc',
  'src/content/inotify/inotify_types.cc',
  'src/content/inotify/mt_inotify.cc',
  'src/content/inotify/scripting_inotify.cc',
  'src/content/layout/builtin_layout.cc',
  'src/content/layout/js_layout.cc',
  'src/content/layout/layout.cc',
  'src/content/onlineservice/curl_online_service.cc',
  'src/content/onlineservice/lastfm_scrobbler.cc',
  'src/content/onlineservice/lastfmlib.cc',
  'src/content/onlineservice/online_service.cc',
  'src/content/onlineservice/online_service_helper.cc',
  'src/content/onlineservice/task_processor.cc',
  'src/content/scripting/import_script.cc',
  'src/content/scripting/js_functions.cc',
  'src/content/scripting/metafile_parser_script.cc',
  'src/content/scripting/parser_script.cc',
  'src/content/scripting/playlist_parser_script.cc',
  'src/content/scripting/script.cc',
  'src/content/scripting/script_property.cc',
  'src/content/scripting/scripting_runtime.cc',
  'src/content/update_manager.cc',
  'src/context.cc',
  'src/contrib/md5.c',
  'src/database/database.cc',
  'src/database/mysql/mysql_database.cc',
  'src/database/mysql/mysql_result.cc',
  'src/database/postgres/pg_result.cc',
  'src/database/postgres/pg_task.cc',
  'src/database/postgres/postgres_database.cc',
  'src/database/search_handler.cc',
  'src/database/sql_database.cc',
  'src/database/sql_table.cc',
  'src/database/sqlite3/sl_result.cc',
  'src/database/sqlite3/sl_task.cc',
  'src/database/sqlite3/sqlite_config.cc',
  'src/database/sqlite3/sqlite_database.cc',
  'src/exceptions.cc',
  'src/iohandler/buffered_io_handler.cc',
  'src/iohandler/curl_io_handler.cc',
  'src/iohandler/file_io_handler.cc',
  'src/iohandler/io_handler.cc',
  'src/iohandler/io_handler_buffer_helper.cc',
  'src/iohandler/io_handler_chainer.cc',
  'src/iohandler/mem_io_handler.cc',
  'src/iohandler/process_io_handler.cc',
  'src/metadata/exiv2_handler.cc',
  'src/metadata/ffmpeg_handler.cc',
  'src/metadata/ffmpeg_thumbnailer_handler.cc',
  'src/metadata/libexif_handler.cc',
  'src/metadata/matroska_handler.cc',
  'src/metadata/metacontent_handler.cc',
  'src/metadata/metadata_enums.cc',
  'src/metadata/metadata_handler.cc',
  'src/metadata/metadata_service.cc',
  'src/metadata/resolution.cc',
  'src/metadata/taglib_handler.cc',
  'src/metadata/wavpack_handler.cc',
  'src/request_handler/device_description_handler.cc',
  'src/request_handler/file_request_handler.cc',
  'src/request_handler/request_handler.cc',
  'src/request_handler/ui_handler.cc',
  'src/request_handler/upnp_desc_handler.cc',
  'src/request_handler/url_request_handler.cc',
  'src/server.cc',
  'src/subscription_request.cc',
  'src/transcoding/transcode_dispatcher.cc',
  'src/transcoding/transcode_ext_handler.cc',
  'src/transcoding/transcode_handler.cc',
  'src/upnp/client_manager.cc',
  'src/upnp/compat.cc',
  'src/upnp/conn_mgr_service.cc',
  'src/upnp/cont_dir_service.cc',
  'src/upnp/headers.cc',
  'src/upnp/mr_reg_service.cc',
  'src/upnp/quirks.cc',
  'src/upnp/upnp_service.cc',
  'src/upnp/xml_builder.cc',
  'src/util/generic_task.cc',
  'src/util/grb_fs.cc',
  'src/util/grb_net.cc',
  'src/util/grb_time.cc',
  'src/util/jpeg_resolution.cc',
  'src/util/logger.cc',
  'src/util/mime.cc',
  'src/util/process_executor.cc',
  'src/util/string_converter.cc',
  'src/util/thread_executor.cc',
  'src/util/timer.cc',
  'src/util/tools.cc',
  'src/util/url.cc',
  'src/util/url_utils.cc',
  'src/web/action.cc',
  'src/web/add.cc',
  'src/web/add_object.cc',
  'src/web/auth.cc',
  'src/web/clients.cc',
  'src/web/config_load.cc',
  'src/web/config_save.cc',
  'src/web/containers.cc',
  'src/web/directories.cc',
  'src/web/edit_load.cc',
  'src/web/edit_save.cc',
  'src/web/files.cc',
  'src/web/items.cc',
  'src/web/page_request.cc',
  'src/web/remove.cc',
  'src/web/session_manager.cc',
  'src/web/tasks.cc',
  'src/web/web_autoscan.cc',
  'src/web/web_request_handler.cc',
  'src/web/web_update.cc',
)

libgerbera = static_library(
  'gerbera',
  libgerbera_files,
  include_directories: libgerbera_incdir,
  dependencies: libgerbera_deps,
  cpp_args: libgerbera_args,
)

libgerbera_dep = declare_dependency(
  include_directories: libgerbera_incdir,
  link_with: libgerbera,
  dependencies: libgerbera_deps,
  compile_args: libgerbera_args,
)

gerbera = executable(
  'gerbera',
  'src/main.cc',
  dependencies: libgerbera_dep,
  install: true,
)

install_data(
  'src/database/mysql/mysql.sql',
  'src/database/mysql/mysql-upgrade.xml',
  'src/database/sqlite3/sqlite3.sql',
  'src/database/sqlite3/sqlite3-upgrade.xml',
  'src/database/postgres/postgres.sql',
  'src/database/postgres/postgres-upgrade.xml',
)

install_subdir(
  'scripts/js',
  install_dir: 'share/gerbera',
)
install_subdir(
  'web',
  install_dir: 'share/gerbera',
)

install_man('doc/gerbera.1')

subdir('test')
