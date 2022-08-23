utilities_src = ['controllers.cc', 'decoders.cc', 'pcie.c', 'pcie-open.cc', 'printer.cc', 'sdb.cc', 'util.cc']
utilities_lib = static_library('utilities', utilities_src, dependencies: [sdbfs], install: true)
utilities_inc = include_directories('.')
utilities = declare_dependency(link_with: utilities_lib, include_directories: utilities_inc)

install_headers(['controllers.h', 'decoders.h', 'defer.h', 'pcie-open.h', 'pcie.h', 'util_sdb.h'], subdir: 'decode-reg')
