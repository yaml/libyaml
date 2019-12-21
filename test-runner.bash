# shellcheck disable=2001,2154,2207

run-tests() {
  whitelist_file=$1; shift

  if [[ $# -gt 0 ]]; then
    ids=("$@")
  else
    ids=($(cut -d: -f1 < "$whitelist_file"))
  fi

  count=0
  for id in "${ids[@]}"; do
    dir=data/$id
    label="$id: $(< "$dir/===")"
    [[ -e $dir/in.yaml ]] || continue

    run-test "$dir"

    if $ok; then
      echo "ok $((++count)) $label"
    else
      echo "not ok $((++count)) $label"
      echo "$output" | sed 's/^/# /'
    fi
  done

  echo "1..$count"
}
