#!/bin/sh

set -e

main() {
  bootstrap

  make test-all

  clean

  cmake .

  make
  make test
}

bootstrap() {
  clean

  ./bootstrap
  ./configure
}

clean() {
  git clean -d -x -f
  rm -fr libyaml-test
}

main "$@"
