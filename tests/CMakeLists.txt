
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
  run-loader
  run-parser
  run-scanner
  test-reader
  test-version
  )
  add_yaml_executable(${name})
endforeach()

add_test(NAME version COMMAND test-version)
add_test(NAME reader COMMAND test-reader)

