
cmake_minimum_required(VERSION 3.0)
project (yaml C)
include(GNUInstallDirs)

set (YAML_VERSION_MAJOR 0)
set (YAML_VERSION_MINOR 1)
set (YAML_VERSION_PATCH 7)
set (YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

#
# Build library
#
set(SRCS
  src/api.c
  src/dumper.c
  src/emitter.c
  src/loader.c
  src/parser.c
  src/reader.c
  src/scanner.c
  src/writer.c
  )

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

add_definitions(-DHAVE_CONFIG_H)
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(config_h ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(
  cmake/config.h.in
  ${config_h}
  )

option(YAML_BUILD_STATIC "Build static library" TRUE)
if(YAML_BUILD_STATIC)
  add_library(yaml-static STATIC ${SRCS})
  set_target_properties(yaml-static PROPERTIES OUTPUT_NAME yaml)
  if(MSVC)
    set_target_properties(yaml-static PROPERTIES PREFIX "lib")
    # compiler: import, runtime, static
    # msvc: yaml.lib, yaml.dll, libyaml.lib
    # mingw: libyaml.dll.a, libyaml.dll, libyaml.a
  endif()
  set(YAML_LIB_TARGET yaml-static)
  target_compile_definitions(yaml-static
    PUBLIC
      YAML_DECLARE_EXPORT=1
      YAML_DECLARE_STATIC=1
  )
  list(APPEND YAML_INSTALL_TARGETS yaml-static)
endif()

option(YAML_BUILD_SHARED "Build shared library" TRUE)
if(YAML_BUILD_SHARED)
  add_library(yaml SHARED ${SRCS})
  set(YAML_LIB_TARGET yaml)
  set_target_properties(yaml PROPERTIES DEFINE_SYMBOL YAML_DECLARE_EXPORT)
  list(APPEND YAML_INSTALL_TARGETS yaml)
endif()

if(YAML_BUILD_SHARED AND YAML_BUILD_STATIC)
  set_target_properties(yaml PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  set_target_properties(yaml-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif()

#
# Install rules
#
install(
  FILES
    include/yaml.h
    ${config_h}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Development
  )

install(
  TARGETS ${YAML_INSTALL_TARGETS}
  EXPORT yamlTargets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Development
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Development
  )

#
# Add tests
#
include(CTest) # This module defines BUILD_TESTING option
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

#
# Generate 'yamlConfig.cmake', 'yamlConfigVersion.cmake' and 'yamlTargets.cmake'
#
include(CMakePackageConfigHelpers)

# Configure 'yamlConfig.cmake' for a build tree
set(config_file ${PROJECT_BINARY_DIR}/yamlConfig.cmake)
configure_package_config_file(
  yamlConfig.cmake.in
  ${config_file}
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

# Configure 'yamlTargets.cmake' for a build tree
export(TARGETS yaml
  FILE ${PROJECT_BINARY_DIR}/yamlTargets.cmake
  )

# Configure and install 'yamlConfig.cmake' for an install tree
set(CONFIG_DIR_CONFIG ${INSTALL_CMAKE_DIR})
set(install_config_file ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/yamlConfig.cmake )
configure_package_config_file(
  yamlConfig.cmake.in
  ${install_config_file}
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
install(
  FILES ${install_config_file}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake COMPONENT Development
  )

# Configure and install 'yamlTargets.cmake' for an install tree
install(EXPORT yamlTargets
  FILE yamlTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
  COMPONENT Development
  )

# Configure 'yamlConfigVersion.cmake' for a build tree
set(config_version_file ${PROJECT_BINARY_DIR}/yamlConfigVersion.cmake)
write_basic_package_version_file(
    ${config_version_file}
    VERSION ${YAML_VERSION_STRING}
    COMPATIBILITY AnyNewerVersion
)
# ... and install for an install tree
install(
  FILES ${config_version_file}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake COMPONENT Development
  )
