
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

const char *
yaml_get_version_string(void)
{
    return YAML_VERSION_STRING;
}

void
yaml_get_version(int *major, int *minor, int *patch)
{
    *major = YAML_VERSION_MAJOR;
    *minor = YAML_VERSION_MINOR;
    *patch = YAML_VERSION_PATCH;
}

