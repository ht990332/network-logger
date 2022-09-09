project('network-logger', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

glibdep = dependency('glib-2.0')
giodep = dependency('gio-2.0')
libeventdep = dependency('libevent')

exe = executable('network_logger', 'network_logger.c',
  dependencies: [glibdep, giodep, libeventdep],
  install : true)

test('basic', exe)
