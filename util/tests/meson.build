test_util_src = ['test-util.cc']
test_util_lib = static_library('test-util', test_util_src, dependencies: [utilities])

copy_test = executable('copy-test', 'copy-test.cc', link_with: [test_util_lib], dependencies: [thread_dep, utilities])
test('copy-test', copy_test)

list_of_keys_test = executable('list-of-keys-test', 'list-of-keys-test.cc', dependencies: [utilities, catch2])
test('list-of-keys-test', list_of_keys_test)

fixed_test = executable('fixed-test', 'fixed-test.cc', dependencies: [utilities, catch2])
test('fixed-test', fixed_test)

decoders_test = executable('decoders-test', 'decoders-test.cc', dependencies: [utilities, catch2])
test('decoders_test', decoders_test)
