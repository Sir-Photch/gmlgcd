project('gmlgcd', 'c',
  version : '1.3-devel',
  default_options : ['warning_level=3']
)

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
  test('util-strnchr', find_program('tests/util-strnchr.fish'))
endif

executable(
  'gmlgcd', 
  sources: ['main.c', 'log.c', 'fcgi.c', 'comment.c', 'quarantine.c', 'appstate.c', 'config.c', 'sandbox.c', 'util.c'],
  dependencies: dependencies,
  install : true
)

install_man('gmlgcd.8')
