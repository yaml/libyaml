# shellcheck disable=2001,2154,2207

set -e -u -o pipefail

run-tests() (
  root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

  source "$LIBYAML_TEST_SUITE_ENV"

  cd "$root/.."

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

declare -a parser_list parser_error_list emitter_list
whitelist() {
  if [[ $# -eq 1 && $1 == all ]]; then
    all=( $(ls data | sort) )
    parser_list=( ${all[@]} )
    parser_error_list=( ${all[@]} )
    emitter_list=( ${all[@]} )
    return
  fi

  test_type=$1; shift
  list_name=${test_type/-/_}_list
  declare -n list=$list_name

  local all="${list[*]}"

  if [[ $1 == *:\ * ]]; then
    white=( $(echo "$1" | cut -d: -f1) )
  else
    white=( "$@" )
  fi

  for id in ${white[@]}; do
    blacklist "$test_type" "$id"
    all="$all $id"
  done

  eval "$list_name=( $all )"
}

blacklist() {
  if [[ $# -eq 1 && $1 == all ]]; then
    parser_list=()
    parser_error_list=()
    emitter_list=()
    return
  fi

  test_type=$1; shift
  list_name=${test_type/-/_}_list
  declare -n list=$list_name

  local all="${list[*]}"

  if [[ $1 == *:\ * ]]; then
    black=( $(echo "$1" | cut -d: -f1) )
  else
    black=( "$@" )
  fi

  for id in ${black[@]}; do
    all=${all/$id/}
  done

  eval "$list_name=( $all )"
}

die() { echo "$*" >&2; exit 1; }
