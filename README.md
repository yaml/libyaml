# How to make a release

## Versioning

Update libyaml version in:
* announcement.msg
* CMakeLists.txt
  * `YAML_VERSION_MAJOR`, `YAML_VERSION_MINOR`, `YAML_VERSION_PATCH`
* appveyor.yml
* configure.ac
  * `YAML_MAJOR`, `YAML_MINOR`, `YAML_PATCH`, `YAML_RELEASE`, `YAML_CURRENT`, `YAML_REVISION`

## Create dist archives

Make sure you have a clean git repository (no changed files). The following
process will clone your current git directory.

Run

    make docker-dist

in the repository root or

    make libyaml-dist

in packaging/docker.

It will create a docker image and run `make dist` in the container to create
a tarball written to packaging/docker/output.
It will also create a zipfile.

## Update pyyaml.org

Put the resulting tarball/zip in pyyaml.org/download/libyaml and run

    make update
