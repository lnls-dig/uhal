project(
  'decode-reg',
  ['c','cpp'],
  version: '0.0.1',
  default_options: ['cpp_std=c++17', 'warning_level=3',],
)

subdir('third_party')

cc = meson.get_compiler('c')
pcidevice = cc.find_library('pcidriver', required: true)
thread_dep = dependency('threads')

executable(
  'decode-reg',
  ['decode-reg.cc', 'pcie.c', 'decoders.cc', 'printer.cc', 'acq.cc', 'util.cc'],
  dependencies: [pcidevice, thread_dep, docopt],
  install: true,
)
