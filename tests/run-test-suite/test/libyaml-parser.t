#!/usr/bin/env bash

set -e

if [[ $# -gt 0 ]]; then
  ids=("$@")
else
  ids=(`find data | grep '/===$' | cut -d/ -f2 | sort`)
fi

count=0
for id in "${ids[@]}"; do
  dir="data/$id"
  label="$id: $(< $dir/===)"
  [[ -e "$dir/in.yaml" ]] || continue
  if grep "$id" test/libyaml-parser.skip >/dev/null; then
    echo "ok $((++count)) # SKIP $label"
    continue
  fi
  ./src/libyaml-parser "$dir/in.yaml" > /tmp/test.out || {
    (
      cat "$dir/in.yaml"
      cat "$dir/test.event"
    ) | sed 's/^/# /'
  }
  ok=true
  output="$(${DIFF:-diff} -u $dir/test.event /tmp/test.out)" || ok=false
  if $ok; then
    echo "ok $((++count)) $label"
  else
    echo "not ok $((++count)) $label"
    echo "$output" | sed 's/^/# /'
  fi
done

echo "1..$count"
