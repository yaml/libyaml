#!/bin/sh

set -e

main() {
  clean

  ./bootstrap
  ./configure
  make test-all

  clean

  cmake .
  make
  make test

  clean
}

clean() {
  git clean -d -x -f
  rm -fr tests/run-test-suite/data
}

main "$@"
