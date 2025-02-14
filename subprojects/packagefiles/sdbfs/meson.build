project(
  'fpga-config-space',
  ['c'],
  default_options: ['c_std=c17', 'warning_level=3',],
)

sdbfs_inc = include_directories(['sdbfs/lib', 'sdbfs/include/linux'])
sdbfs_lib = static_library(
  'sdbfs',
  ['sdbfs/lib/access.c', 'sdbfs/lib/glue.c'],
  include_directories: sdbfs_inc,
)
sdbfs_dep = declare_dependency(link_with: sdbfs_lib, include_directories: sdbfs_inc)
