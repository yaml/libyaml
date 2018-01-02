#!/usr/bin/env bash

set -e

if [[ $# -gt 0 ]]; then
  ids=("$@")
else
  ids=($(cut -d: -f1 < list/libyaml-parser-error.list))
fi

count=0
for id in "${ids[@]}"; do
  dir="data/$id"
  label="$id: $(< $dir/===)"
  [[ -e "$dir/in.yaml" ]] || continue
  ok=true
  ../../tests/run-parser-test-suite "$dir/in.yaml" > /tmp/test.out 2>&1 || ok=false
  if $ok; then
    echo "not ok $((++count)) $label"
    sed 's/^/# /' /tmp/test.out
  else
    echo "ok $((++count)) $label"
  fi
done

echo "1..$count"
