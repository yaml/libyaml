#!/usr/bin/env bash

set -e

if [[ $# -gt 0 ]]; then
  ids=("$@")
else
  ids=($(cut -d: -f1 < test/libyaml-emitter.list))
fi

# Some environments like on OS X, the shell resets the following vars, so we
# work around it like so:
export LD_LIBRARY_PATH="${MY_LD_LIBRARY_PATH:?}"
export DYLD_LIBRARY_PATH="${MY_LD_LIBRARY_PATH:?}"

count=0
for id in "${ids[@]}"; do
  dir="data/$id"
  label="$id: $(< $dir/===)"
  [[ -e "$dir/in.yaml" ]] || continue
  want="$dir/out.yaml"
  [[ -e $want ]] || want="$dir/in.yaml"
  ./src/libyaml-emitter "$dir/test.event" > /tmp/test.out || {
    (
      cat "$dir/test.event"
      cat "$want"
    ) | sed 's/^/# /'
  }
  ok=true
  output="$(${DIFF:-diff} -u $want /tmp/test.out)" || ok=false
  if $ok; then
    echo "ok $((++count)) $label"
  else
    echo "not ok $((++count)) $label"
    echo "$output" | sed 's/^/# /'
  fi
done

echo "1..$count"
