# Minimal CMake project for building a static library under Windows.

cmake_minimum_required (VERSION 2.8)

if (NOT DEFINED CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif ()

project (yaml C)

set (YAML_VERSION_MAJOR 0)
set (YAML_VERSION_MINOR 1)
set (YAML_VERSION_PATCH 7)
set (YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

file (GLOB SRC src/*.c)
install (FILES include/yaml.h DESTINATION include)

include_directories (include win32)
add_library (yaml SHARED ${SRC})
target_compile_definitions (yaml PRIVATE -DYAML_DECLARE_EXPORT -DHAVE_CONFIG_H)
install (TARGETS yaml RUNTIME DESTINATION bin LIBRARY DESTINATION lib${LIB_SUFFIX} ARCHIVE DESTINATION lib${LIB_SUFFIX})

add_library (yaml_static STATIC ${SRC})
target_compile_definitions (yaml_static PRIVATE -DYAML_DECLARE_STATIC -DHAVE_CONFIG_H)
install (TARGETS yaml_static ARCHIVE DESTINATION lib${LIB_SUFFIX})

add_executable (test-version tests/test-version.c)
target_link_libraries(test-version yaml)
add_test(NAME version COMMAND test-version)

add_executable (test-reader tests/test-reader.c)
target_link_libraries(test-reader yaml)
add_test(NAME reader COMMAND test-reader)

enable_testing()
