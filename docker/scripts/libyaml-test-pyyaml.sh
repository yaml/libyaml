#!/bin/bash

set -e
set -x

# Create tarball
cd /tmp
cp -rp /output/libyaml.git .
#git clone https://github.com/yaml/libyaml.git
cd libyaml.git
./bootstrap
./configure
make
make test
make test-all
make install
ldconfig

cd /tmp
git clone https://github.com/yaml/pyyaml.git
cd pyyaml
python --version
python setup.py test

echo "python 2 pyyaml successful"

git clean -xdf
python3 setup.py test

echo "python 2 & 3 pyyaml successful"
