#!/bin/bash

set -e

if test "$platform" = "bash"; then
    echo Building on Ubuntu Linux x86_64
    sudo apt-get install build-essential autoconf libtool
elif test "$platform" = "msys32"; then
    echo Building on Msys2 i686
elif test "$platform" = "mingw32"; then
    echo Building on MinGW-w64 i686
    echo FIXME: unsupported yet
    exit 1
else
    echo ERROR: unknown platform "$platform"
    exit 1
fi

./bootstrap
./configure
make
make check
