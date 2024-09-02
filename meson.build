project('gmlgcd', 'c',
  version : '1.0',
  default_options : ['warning_level=3']
)

fs = import('fs')

configure_file(
  input:  'version.h.in',
  output: 'version.h',
  configuration: {
    'version': meson.project_version()
  }
)

if get_option('buildtype') in ['debug', 'debugoptimized']
  add_project_arguments('-DDEBUG_BUILD', language: 'c')
endif

if meson.get_compiler('c').get_id() != 'gcc'
  add_project_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'c')
endif

dependencies = [
  dependency('libevent'),
  dependency('libconfuse')
]

if host_machine.system() == 'linux'
  dependencies += dependency('libbsd')

  executable('test_util', sources: ['util.c', 'tests/util.c'], install: false)
  test('util-trim', find_program('tests/util-trim.fish'))
  test('util-path-combine', find_program('tests/util-path-combine.fish'))
endif

executable(
  'gmlgcd', 
  sources: ['main.c', 'log.c', 'fcgi.c', 'comment.c', 'quarantine.c', 'appstate.c', 'config.c', 'sandbox.c', 'util.c'],
  dependencies: dependencies,
  install : true
)

systemddir = get_option('systemddir')
prefix = get_option('prefix')
sysconfdir = get_option('sysconfdir')

install_man('gmlgcd.8')
install_data('template.gmi', install_dir: prefix + '/share/doc/gmlgcd')

if not fs.exists('/' + sysconfdir + '/gmlgcd.conf')
  install_data('gmlgcd.conf', install_dir: sysconfdir)
else
  message('/' + sysconfdir + '/gmlgcd.conf exists, not overwriting')
endif

if systemddir == ''
  install_data('gmlgcd.service', install_dir: systemddir)
endif
