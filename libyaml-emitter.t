#!/usr/bin/env bash

# shellcheck disable=1090,2034

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

source "$root"/test-runner.bash

run-test() {
  dir=$1
  ok=true
  want=$dir/out.yaml
  [[ -e $want ]] || want="$dir/in.yaml"
  ../../tests/run-emitter-test-suite "$dir/test.event" > /tmp/test.out || {
    (
      cat "$dir/test.event"
      cat "$want"
    ) | sed 's/^/# /'
  }
  output="$(${DIFF:-diff} -u "$want" /tmp/test.out)" || ok=false
}

run-tests "$root/blacklist/libyaml-emitter" "$@"
