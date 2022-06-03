project(
  'decode-reg',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['c_std=gnu11', 'cpp_std=c++17', 'warning_level=3',],
)

subdir('third_party')

cc = meson.get_compiler('c')
thread_dep = dependency('threads')
argparse = dependency('argparse', fallback: ['argparse', 'argparse_dep'])

subdir('util')
subdir('modules')
subdir('app')
