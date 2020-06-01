#!/usr/bin/env bash

# shellcheck disable=1090,2034

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

source "$root"/test-runner.bash

check-test() {
  id=$1
  t=data/$id

  grep "$id" "$root/blacklist/libyaml-parser" >/dev/null && return 1
  [[ -e $t/error ]] && return 1

  return 0
}

run-test() {
  dir=$1
  ok=true

  ../../tests/run-parser-test-suite "$dir/in.yaml" > /tmp/test.out || {
    (
      cat "$dir/in.yaml"
      cat "$dir/test.event"
    ) | sed 's/^/# /'
  }

  output=$(${DIFF:-diff} -u "$dir/test.event" /tmp/test.out) || ok=false
}

run-tests "$@"
