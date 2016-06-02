# Minimal CMake project for building a static library under Windows.

cmake_minimum_required (VERSION 2.8)
project (yaml C)

set (YAML_VERSION_MAJOR 0)
set (YAML_VERSION_MINOR 1)
set (YAML_VERSION_PATCH 7)
set (YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

file (GLOB SRC src/*.c)

include_directories (include win32)
add_library (yaml SHARED ${SRC})
set_target_properties(yaml PROPERTIES COMPILE_FLAGS "-DYAML_DECLARE_EXPORT -DHAVE_CONFIG_H")
add_library (yaml_static STATIC ${SRC})
set_target_properties(yaml_static PROPERTIES COMPILE_FLAGS "-DYAML_DECLARE_STATIC -DHAVE_CONFIG_H")