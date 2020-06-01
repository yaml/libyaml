#!/usr/bin/env bash

# shellcheck disable=1090,2034

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

source "$root"/test-runner.bash

check-test() {
  id=$1
  t=data/$id

  [[ " ${parser_error_list[*]} " == *\ $id\ * ]] || return 1
  [[ -e $t/error ]] || return 1

  return 0
}

run-test() {
  dir=$1
  ok=false

  ../../tests/run-parser-test-suite "$dir/in.yaml" > /tmp/test.out 2>&1 || ok=true

  $ok || output=$(< /tmp/test.out)
}

run-tests "$@"
