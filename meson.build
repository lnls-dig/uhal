project(
  'μHAL',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['c_std=gnu11', 'cpp_std=c++17', 'warning_level=3',],
)

subdir('third_party')

thread_dep = dependency('threads')
argparse = dependency('argparse', fallback: 'argparse')

header_dir = 'uhal'

subdir('util')
subdir('modules')
subdir('app')
