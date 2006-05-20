#include <yaml/yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int
main(void)
{
    int major, minor, patch;
    char buf[64];

    yaml_get_version(&major, &minor, &patch);
    sprintf(buf, "%d.%d.%d", major, minor, patch);
    assert(strcmp(buf, yaml_get_version_string()) == 0);
    assert(yaml_check_version(major+1, minor, patch) == 0);
    assert(yaml_check_version(major, minor+1, patch) == 0);
    assert(yaml_check_version(major, minor, patch+1) == 1);
    assert(yaml_check_version(major, minor, patch) == 1);
    assert(yaml_check_version(major, minor, patch-1) == 0);

    return 0;
}
