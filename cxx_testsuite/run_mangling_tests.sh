#!/bin/bash
# Run C++ ABI name mangling tests for DaveCC.
set -uo pipefail

DAVECC=""
SUITE_ROOT=""
TESTS_DIR="tests/mangling"
INCLUDE_DIR=""
DEFAULT_STD="-std=c++20"

usage() {
  echo "usage: $0 --davecc PATH [--suite-root PATH] [--tests-dir PATH] [--std FLAG]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --tests-dir) TESTS_DIR=$2; shift 2 ;;
    --include-dir) INCLUDE_DIR=$2; shift 2 ;;
    --std) DEFAULT_STD=$2; shift 2 ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

if [ -z "$DAVECC" ]; then
  usage
fi

resolve_runfile() {
  local path=$1
  if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
    local rooted="${TEST_SRCDIR}/${TEST_WORKSPACE}/${path}"
    if [ -e "$rooted" ]; then
      echo "$rooted"
      return
    fi
  fi
  echo "$path"
}

DAVECC=$(resolve_runfile "$DAVECC")
if [ -n "$SUITE_ROOT" ]; then
  SUITE_ROOT=$(resolve_runfile "$SUITE_ROOT")
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  SUITE_ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE}/cxx_testsuite"
else
  SUITE_ROOT="cxx_testsuite"
fi
if [ -n "$INCLUDE_DIR" ]; then
  INCLUDE_DIR=$(resolve_runfile "$INCLUDE_DIR")
else
  INCLUDE_DIR="$SUITE_ROOT/include"
fi

work=$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-mangling.XXXXXX")
trap 'rm -rf "$work"' EXIT

read_run_args() {
  local file=$1
  local line
  RUN_ARGS=()
  while IFS= read -r line; do
    case "$line" in
      "// RUN:"*) eval "RUN_ARGS+=(${line#// RUN:})" ;;
    esac
  done < "$file"
}

print_expected_asm() {
  local file=$1
  local line
  while IFS= read -r line; do
    case "$line" in
      "// EXPECT-ASM:"*) printf '%s\n' "${line#// EXPECT-ASM: }" ;;
    esac
  done < "$file"
}

pass=0
fail=0

echo "=== cxx_testsuite mangling: tests=$TESTS_DIR ==="
echo "davecc=$DAVECC"
echo

for src in "$SUITE_ROOT/$TESTS_DIR"/pass/*.cpp; do
  [ -e "$src" ] || continue
  base=$(basename "$src")
  log="$work/$base.log"
  out="$work/$base.s"
  read_run_args "$src"
  args=(-target pcode -S "$DEFAULT_STD" -isystem "$INCLUDE_DIR")
  args+=("${RUN_ARGS[@]}")
  status=0
  "$DAVECC" "${args[@]}" "$src" -o "$out" >"$log" 2>&1 || status=$?
  if [ "$status" -ne 0 ] || grep -q "error:" "$log"; then
    echo "FAIL pass/$base"
    sed 's/^/  /' "$log" | head -20
    fail=$((fail + 1))
    continue
  fi
  missing=0
  while IFS= read -r expected; do
    [ -n "$expected" ] || continue
    if ! grep -Fq "$expected" "$out"; then
      echo "FAIL pass/$base missing asm: $expected"
      missing=1
    fi
  done < <(print_expected_asm "$src")
  if [ "$missing" -ne 0 ]; then
    sed 's/^/  /' "$out" | head -80
    fail=$((fail + 1))
    continue
  fi
  echo "ok pass/$base"
  pass=$((pass + 1))
done

echo
echo "=== summary cxx_testsuite mangling ==="
echo "pass=$pass fail=$fail"

if [ "$fail" -ne 0 ]; then
  exit 1
fi
