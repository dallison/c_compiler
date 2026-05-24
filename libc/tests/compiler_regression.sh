#!/bin/bash
# Run compiler regression cases for the given target and optimization level.
set -uo pipefail

if [ "$#" -lt 3 ]; then
  echo "usage: $0 <davecc> <target> <opt-level>" >&2
  exit 2
fi

davecc=$1
target=$2
opt=$3

work=$(mktemp -d "${TMPDIR:-/tmp}/libc_compiler_test.XXXXXX")
trap 'rm -rf "$work"' EXIT

pass=0
fail=0

run_case() {
  local name=$1
  local src=$2
  local expect=$3
  local obj="$work/$name.o"
  set +e
  "$davecc" -target "$target" "$opt" -c -isystem libc/include "$src" -o "$obj" \
    2>"$work/$name.err"
  local status=$?
  set -e

  if [ "$expect" = pass ] && [ "$status" -eq 0 ]; then
    echo "PASS $name"
    pass=$((pass + 1))
    return 0
  fi
  if [ "$expect" = fail ] && [ "$status" -ne 0 ]; then
    echo "XFAIL $name (known failure)"
    pass=$((pass + 1))
    return 0
  fi

  echo "UNEXPECTED $name (status=$status, expected=$expect)"
  sed 's/^/  /' "$work/$name.err" 2>/dev/null || true
  fail=$((fail + 1))
}

case "$opt" in
  -O0)
    run_case if_else libc/tests/compiler/if_else.c pass
    run_case break_in_while libc/tests/compiler/break_in_while.c pass
    ;;
  -O1)
    run_case if_else libc/tests/compiler/if_else.c pass
    run_case memset libc/memset.c pass
    run_case compare_with_zero libc/tests/compiler/compare_with_zero.c pass
    ;;
  *)
    echo "unsupported opt level: $opt" >&2
    exit 2
    ;;
esac

echo "compiler regression summary: pass=$pass fail=$fail"
if [ "$fail" -ne 0 ]; then
  exit 1
fi
