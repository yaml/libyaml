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
make dist

# get the tarball filename
tarball="$(ls yaml-*.tar.gz | head -1)"
dirname="${tarball/.tar.gz/}"

# Copy to output dir
cp "$tarball" /output

# Create zip archive
cd /tmp
cp "/output/$tarball" .
tar xvf "$tarball"
zip -r "/output/$dirname.zip" "$dirname"
