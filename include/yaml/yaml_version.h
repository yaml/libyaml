/**
 * @file yaml_version.h
 * @brief Version information.
 *
 * Do not include yaml_version.h directly.
 */

#ifndef YAML_VERSION_H
#define YAML_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the library version.
 */

const char *
yaml_get_version_string(void);

/**
 * @brief Get the library version numbers.
 */

void
yaml_get_version(int *major, int *minor, int *patch);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_VERSION_H */
