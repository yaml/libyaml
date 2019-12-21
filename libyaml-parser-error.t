#!/usr/bin/env bash

# shellcheck disable=1090,2034

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

source "$root"/test-runner.bash

run-test() {
  dir=$1
  ok=false
  ../../tests/run-parser-test-suite "$dir/in.yaml" > /tmp/test.out 2>&1 || ok=true
  $ok || output=$(< /tmp/test.out)
}

run-tests "$root/blacklist/libyaml-parser-error" "$@"
