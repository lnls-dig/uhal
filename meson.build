project(
  'μHAL',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['c_std=c17', 'cpp_std=c++20', 'warning_level=3',],
)

languages = ['c', 'cpp']

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: languages)

if get_option('pcie_opt')
  if host_machine.cpu_family() == 'x86_64'
    add_project_arguments('-msse4.1', language: languages)
  endif
endif

foreach lang : languages
  compiler = meson.get_compiler(lang)
  foreach arg : ['-Wstrict-aliasing=3', '-Wvla', '-Wsuggest-override']
    if compiler.has_argument(arg)
      add_project_arguments(arg, language: lang)
    endif
  endforeach
endforeach

thread_dep = dependency('threads')
argparse = dependency('argparse', fallback: 'argparse')
catch2 = dependency('catch2-with-main', fallback: 'catch2')
sdbfs = dependency('sdbfs', fallback: 'sdbfs')
ordered_map = dependency('ordered-map', fallback: 'ordered-map')

header_dir = 'uhal'

subdir('include')
subdir('util')
subdir('templates')
subdir('modules')
subdir('app')
