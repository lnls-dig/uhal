docopt_lib = static_library('docopt', 'docopt.cpp/docopt.cpp')
docopt_inc = include_directories('docopt.cpp')
docopt = declare_dependency(link_with: docopt_lib, include_directories: docopt_inc)

sdbfs_inc = include_directories(['sdbfs/lib', 'sdbfs/include/linux'])
sdbfs_lib = static_library(
  'sdbfs',
  ['sdbfs/lib/access.c', 'sdbfs/lib/glue.c'],
  include_directories: sdbfs_inc
)
sdbfs = declare_dependency(link_with: sdbfs_lib, include_directories: sdbfs_inc)
