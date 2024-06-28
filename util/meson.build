utilities_src = [
  'controllers.cc',
  'decoderbase.cc',
  'decoders.cc',
  'pcie-open.cc',
  'pcie.c',
  'printer.cc',
  'sdb.cc',
  'si57x_util.cc',
  'util.cc',
]
utilities_lib = static_library(
  'uhal-utilities',
  utilities_src,
  dependencies: [sdbfs],
  include_directories: ordered_map_inc,
  install: true
)
utilities_inc = include_directories('.')
utilities = declare_dependency(link_with: utilities_lib, include_directories: [utilities_inc, ordered_map_inc])

subdir('tests')

install_headers(
  [
    'controllers.h',
    'decoders.h',
    'pcie-defs.h',
    'pcie-open.h',
    'sdb-defs.h',
    'util_sdb.h',
  ],
  subdir: header_dir
)
