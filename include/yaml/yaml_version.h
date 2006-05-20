#ifndef YAML_VERSION_H
#define YAML_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

const char *
yaml_get_version_string(void);

void
yaml_get_version(int *major, int *minor, int *patch);

int
yaml_check_version(int major, int minor, int patch);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_VERSION_H */
