test_util_src = ['test-util.cc']
test_util_lib = static_library('test-util', test_util_src, dependencies: [utilities])

copy_test = executable('copy-test', 'copy-test.cc', link_with: [test_util_lib], dependencies: [thread_dep, utilities])
test('copy-test', copy_test)

tests = [
  'bits-test',
  'controllers-test',
  'decoders-test',
  'fixed-test',
  'list-of-keys-test',
  'si57x-test',
]
foreach test_name : tests
  exe = executable(test_name, test_name + '.cc', dependencies: [utilities, catch2])
  test(test_name, exe)
endforeach
