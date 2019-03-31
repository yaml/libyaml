#!/bin/bash

set -e
set -x

cd /tmp
cp -rp /output/libyaml.git .
#git clone https://github.com/yaml/libyaml.git
cd libyaml.git
./bootstrap
./configure
make dist

cp yaml-*.tar.gz /output

