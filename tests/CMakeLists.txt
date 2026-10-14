
function(add_yaml_executable name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} yaml)
endfunction()

foreach(name IN ITEMS
  example-deconstructor
  example-deconstructor-alt
  example-reformatter
  example-reformatter-alt
  run-dumper
  run-emitter
  run-emitter-test-suite
  run-loader
  run-parser
  run-parser-test-suite
  run-scanner
  test-nesting
  test-reader
  test-version
  )
  add_yaml_executable(${name})
endforeach()

add_test(NAME version COMMAND test-version)
add_test(NAME reader COMMAND test-reader)
add_test(NAME nesting COMMAND test-nesting)

# Regression test for allocation-failure handling in the loader: the
# library sources are compiled into the test with the allocator renamed,
# so the test can inject allocation failures and count outstanding blocks
# portably, without LD_PRELOAD or linker-specific tricks.
foreach(src IN LISTS SRCS)
  list(APPEND SRCS_ABS ${PROJECT_SOURCE_DIR}/${src})
endforeach()

add_executable(test-load-error test-load-error.c test-alloc.c ${SRCS_ABS})
target_compile_definitions(test-load-error PRIVATE
  HAVE_CONFIG_H YAML_DECLARE_STATIC)
target_include_directories(test-load-error PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/src)
set_property(SOURCE ${SRCS_ABS} APPEND PROPERTY COMPILE_DEFINITIONS
  malloc=test_malloc realloc=test_realloc free=test_free strdup=test_strdup)
add_test(NAME load-error COMMAND test-load-error)

