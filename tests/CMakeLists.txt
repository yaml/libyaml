
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
  test-realloc
  test-version
  )
  add_yaml_executable(${name})
endforeach()

target_include_directories(test-realloc PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_test(NAME version COMMAND test-version)
add_test(NAME reader COMMAND test-reader)
add_test(NAME nesting COMMAND test-nesting)
add_test(NAME realloc COMMAND test-realloc)
