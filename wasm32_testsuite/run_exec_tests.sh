#!/bin/bash
#
# Differential execution tests for the wasm32 backend.
#
# Each test is a self-contained C program whose exit status is its answer.
# The same program is built twice, once by the host clang and once by davecc
# for wasm32, and the two answers have to agree.  Comparing against a second
# compiler rather than a recorded value means a test cannot silently bless
# whatever the backend currently does.
#
# Usage: run_exec_tests.sh [davecc] [test.c ...]

set -u

DAVECC=${1:-bazel-bin/davecc}
shift 2>/dev/null || true

if [ $# -eq 0 ]; then
  set -- "$(dirname "$0")"/tests/*.c
fi

for tool in wasm-validate wasmtime clang; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "run_exec_tests.sh: $tool is not installed" >&2
    exit 2
  fi
done

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

pass=0
fail=0

for source in "$@"; do
  name=$(basename "$source" .c)

  if ! clang -w -o "$work/$name.native" "$source" 2>/dev/null; then
    echo "SKIP $name: the host compiler rejected it"
    continue
  fi
  "$work/$name.native"
  want=$?

  if ! error=$("$DAVECC" -target wasm32 -c "$source" -o "$work/$name.wasm" 2>&1); then
    echo "FAIL $name: $(echo "$error" | head -1)"
    fail=$((fail + 1))
    continue
  fi

  if ! error=$(wasm-validate "$work/$name.wasm" 2>&1); then
    echo "FAIL $name: invalid module: $(echo "$error" | head -1)"
    fail=$((fail + 1))
    continue
  fi

  got=$(wasmtime run --invoke main "$work/$name.wasm" 2>/dev/null)
  # A process exit status is only the low eight bits, so mask what wasm
  # returned the same way before comparing.
  if [ "$((got & 255))" = "$want" ]; then
    pass=$((pass + 1))
  else
    echo "FAIL $name: want $want, got $got"
    fail=$((fail + 1))
  fi
done

echo "wasm32 exec tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
