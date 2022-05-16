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

modules = ['acq.cc', 'lamp.cc']

executable(
  'decode-reg',
  ['decode-reg.cc', 'pcie.c', 'decoders.cc', 'printer.cc', 'pcie-open.cc', 'util.cc'] + modules,
  dependencies: [thread_dep, argparse, sdbfs],
  install: true,
)
