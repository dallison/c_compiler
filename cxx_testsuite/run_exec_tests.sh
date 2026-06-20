#!/bin/bash
# Run C++ execution tests with DaveCC + target libc + interpreter.
set -o pipefail

DAVECC=""
TARGET=""
LIBC=""
INTERPRETER=""
TESTS_DIR="tests/exec"
SUITE_ROOT=""
TIMEOUT=30
declare -a INTERP_ARGS=()
declare -a COMPILE_ARGS=()

usage() {
  echo "usage: $0 --davecc PATH --target NAME --libc PATH --interpreter PATH [--tests-dir PATH]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --target) TARGET=$2; shift 2 ;;
    --libc) LIBC=$2; shift 2 ;;
    --interpreter) INTERPRETER=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --tests-dir) TESTS_DIR=$2; shift 2 ;;
    --timeout) TIMEOUT=$2; shift 2 ;;
    --interp-arg) INTERP_ARGS+=("$2"); shift 2 ;;
    --compile-arg) COMPILE_ARGS+=("$2"); shift 2 ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

if [ -z "$DAVECC" ] || [ -z "$TARGET" ] || [ -z "$LIBC" ] ||
   [ -z "$INTERPRETER" ]; then
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
LIBC=$(resolve_runfile "$LIBC")
INTERPRETER=$(resolve_runfile "$INTERPRETER")
if [ -n "$SUITE_ROOT" ]; then
  SUITE_ROOT=$(resolve_runfile "$SUITE_ROOT")
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  SUITE_ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE}/cxx_testsuite"
else
  SUITE_ROOT="cxx_testsuite"
fi

INCLUDE_DIR="$SUITE_ROOT/../libc/include"
if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  INCLUDE_DIR="${TEST_SRCDIR}/${TEST_WORKSPACE}/libc/include"
fi

declare -a TIMEOUT_CMD=()
if command -v gtimeout >/dev/null 2>&1; then
  TIMEOUT_CMD=(gtimeout "$TIMEOUT")
elif command -v timeout >/dev/null 2>&1; then
  TIMEOUT_CMD=(timeout "$TIMEOUT")
fi

work=$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-exec.XXXXXX")
trap 'rm -rf "$work"' EXIT

pass=0
fail=0

read_expected_exit() {
  local file=$1
  local line
  EXPECT_EXIT=""
  while IFS= read -r line; do
    case "$line" in
      "// EXPECT_EXIT:"*)
        EXPECT_EXIT="${line#// EXPECT_EXIT: }"
        ;;
    esac
  done < "$file"
}

exit_status_matches() {
  local expected=$1
  local actual=$2
  if [ "$expected" = "nonzero" ]; then
    [ "$actual" -ne 0 ]
    return $?
  fi
  [ "$actual" -eq "$expected" ]
}

echo "=== cxx_testsuite exec: target=$TARGET ==="
echo "davecc=$DAVECC"
echo "libc=$LIBC"
echo "interpreter=$INTERPRETER"
echo

for src in "$SUITE_ROOT/$TESTS_DIR"/*.cpp; do
  [ -e "$src" ] || continue
  base=$(basename "$src")
  read_expected_exit "$src"
  exp="${src}.expected"
  bin="$work/test.bin"
  out="$work/test.out"
  compile_cmd=("$DAVECC" -target "$TARGET" -static -std=c++20
               -isystem "$INCLUDE_DIR")
  compile_cmd+=("${COMPILE_ARGS[@]}" "$src" "$LIBC" -o "$bin")
  if ! "${compile_cmd[@]}" >"$work/compile.log" 2>&1; then
    echo "FAIL $base (compile)"
    sed 's/^/  /' "$work/compile.log" | head -20
    fail=$((fail + 1))
    continue
  fi

  run_cmd=("${TIMEOUT_CMD[@]}" "$INTERPRETER")
  run_cmd+=("${INTERP_ARGS[@]}" "$bin")
  "${run_cmd[@]}" >"$out" 2>"$work/run.err"
  run_status=$?
  if [ -n "$EXPECT_EXIT" ]; then
    if ! exit_status_matches "$EXPECT_EXIT" "$run_status"; then
      echo "FAIL $base (run exit $run_status, expected $EXPECT_EXIT)"
      sed 's/^/  /' "$work/run.err" | head -20
      fail=$((fail + 1))
      continue
    fi
    if [ -f "$exp" ] && ! diff -u "$exp" "$out" >"$work/diff"; then
      echo "FAIL $base (output)"
      sed 's/^/  /' "$work/diff" | head -40
      fail=$((fail + 1))
      continue
    fi
    echo "ok $base"
    pass=$((pass + 1))
    continue
  fi
  if [ "$run_status" -ne 0 ]; then
    echo "FAIL $base (run exit $run_status)"
    sed 's/^/  /' "$work/run.err" | head -20
    fail=$((fail + 1))
    continue
  fi
  if [ -f "$exp" ] && ! diff -u "$exp" "$out" >"$work/diff"; then
    echo "FAIL $base (output)"
    sed 's/^/  /' "$work/diff" | head -40
    fail=$((fail + 1))
    continue
  fi
  echo "ok $base"
  pass=$((pass + 1))
done

echo
echo "=== summary cxx_testsuite exec ==="
echo "pass=$pass fail=$fail"

if [ "$fail" -ne 0 ]; then
  exit 1
fi
