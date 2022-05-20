project(
  'decode-reg',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['cpp_std=c++17', 'warning_level=3',],
)

subdir('third_party')

cc = meson.get_compiler('c')
thread_dep = dependency('threads')
argparse = dependency('argparse', fallback: ['argparse', 'argparse_dep'])

utilities_src = ['decoders.cc', 'pcie.c', 'pcie-open.cc', 'printer.cc', 'util.cc']
utilities_lib = static_library('utilities', utilities_src, dependencies: [sdbfs])
utilities_inc = include_directories('.')
utilities = declare_dependency(link_with: utilities_lib, include_directories: utilities_inc)
subdir('modules')

executable(
  'decode-reg',
  ['decode-reg.cc'],
  dependencies: [thread_dep, argparse, utilities, modules],
  install: true,
)
