# Minimal CMake project for building a static library under Windows.

cmake_minimum_required (VERSION 2.8)
project (yaml C)

set (YAML_VERSION_MAJOR 0)
set (YAML_VERSION_MINOR 1)
set (YAML_VERSION_PATCH 7)
set (YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")
set (PACKAGE_VERSION "${YAML_VERSION_STRING}")

set (YAML_RELEASE  0)
set (YAML_CURRENT  2)
set (YAML_REVISION 5)
set (YAML_AGE      0)
set (YAML_SOVERSION "${YAML_CURRENT}.${YAML_REVISION}.${YAML_AGE}")

file (GLOB SRC src/*.c)

include(GNUInstallDirs)

include_directories (include win32)
add_library (yaml SHARED ${SRC})
set_target_properties(yaml PROPERTIES COMPILE_FLAGS "-DYAML_DECLARE_EXPORT -DHAVE_CONFIG_H"
    VERSION "${YAML_SOVERSION}"
    SOVERSION "${YAML_SOVERSION}"
    OUTPUT_NAME yaml-0
)
set_property(TARGET yaml APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library (yaml_static STATIC ${SRC})
set_target_properties(yaml_static PROPERTIES COMPILE_FLAGS "-DYAML_DECLARE_STATIC -DHAVE_CONFIG_H")
set_property(TARGET yaml_static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_executable (test-version tests/test-version.c)
target_link_libraries(test-version yaml)
add_test(NAME version COMMAND test-version)

add_executable (test-reader tests/test-reader.c)
target_link_libraries(test-reader yaml)
add_test(NAME reader COMMAND test-reader)

enable_testing()

file(RELATIVE_PATH REL_INCLUDE_DIR
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
configure_file(yamlConfig.cmake.in
    "${PROJECT_BINARY_DIR}/yamlConfig.cmake"
    @ONLY
)
configure_file(yamlConfigVersion.cmake.in
    "${PROJECT_BINARY_DIR}/yamlConfigVersion.cmake"
    @ONLY
)
set(prefix ${CMAKE_INSTALL_PREFIX})
set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(exec_prefix "\${prefix}")
configure_file(yaml-0.1.pc.in
    "${PROJECT_BINARY_DIR}/yaml-0.1.pc"
    @ONLY
)

install(FILES
    "${PROJECT_BINARY_DIR}/yamlConfig.cmake"
    "${PROJECT_BINARY_DIR}/yamlConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
install(FILES
    "${PROJECT_BINARY_DIR}/yaml-0.1.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
install(TARGETS yaml yaml_static
    EXPORT yaml-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(EXPORT yaml-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
install(FILES include/yaml.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

export(TARGETS yaml yaml_static FILE yaml-exports.cmake)
