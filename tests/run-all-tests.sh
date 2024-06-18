#!/bin/sh

set -e

main() {
  # Autoconf based in-source build and tests
  clean
  export LDFLAGS="-L/usr/local/opt/icu4c/lib -licuuc"
  export CPPFLAGS="-I/usr/local/opt/icu4c/include"
  ./bootstrap
  ./configure
  make test-all

  # CMake based in-source build and tests
  clean
  export CMAKE_PREFIX_PATH=/usr/local/opt/icu4c
  cmake .
  make
  make test

  clean
}

clean() {
  git clean -d -x -f
  rm -fr tests/run-test-suite
  git worktree prune
}

main "$@"
