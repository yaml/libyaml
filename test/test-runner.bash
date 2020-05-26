# shellcheck disable=2001,2154,2207

set -e -u -o pipefail

run-tests() (
  root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

  cd "$root"

  if [[ $# -gt 0 ]]; then
    ids=("$@")
  else
    ids=($(cd data; printf "%s\n" * | grep '[0-9]'))
  fi

  count=0
  for id in "${ids[@]}"; do
    check-test "$id" || continue

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
)
