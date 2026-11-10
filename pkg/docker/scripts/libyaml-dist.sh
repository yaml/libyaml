#!/usr/bin/env bash

set -ex

cp -r /src /tmp/libyaml
cd /tmp/libyaml
./bootstrap
./configure
make clean
make dist

# Copy to output dir
cp yaml-*.tar.gz yaml-*.zip /src/pkg/docker/output
