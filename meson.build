project(
  'μHAL',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['c_std=gnu17', 'cpp_std=c++20', 'warning_level=3',],
)

languages = ['c', 'cpp']

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


subdir('third_party')

thread_dep = dependency('threads')
argparse = dependency('argparse', fallback: 'argparse')
catch2 = dependency('catch2-with-main', fallback: 'catch2')

header_dir = 'uhal'

subdir('include')
subdir('util')
subdir('modules')
subdir('app')
