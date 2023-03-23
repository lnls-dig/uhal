project(
  'μHAL',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['c_std=gnu11', 'cpp_std=c++17', 'warning_level=3',],
)

if get_option('pcie_opt')
  if host_machine.cpu_family() == 'x86_64'
    add_project_arguments('-msse4.1', language: ['c', 'cpp'])
  endif
endif

add_project_arguments('-Wstrict-aliasing=3', language: ['c', 'cpp'])

subdir('third_party')

thread_dep = dependency('threads')
argparse = dependency('argparse', fallback: 'argparse')

header_dir = 'uhal'

subdir('util')
subdir('modules')
subdir('app')
