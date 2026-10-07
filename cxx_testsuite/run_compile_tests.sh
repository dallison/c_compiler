#!/bin/bash
# Run compile-only C++ tests for DaveCC.
set -uo pipefail

DAVECC=""
SUITE_ROOT=""
TESTS_DIR="tests/lexical"
INCLUDE_DIR=""
DEFAULT_STD="-std=c++20"
TERMINATE=false
EXPECT_FAIL_FILE=""

usage() {
  echo "usage: $0 --davecc PATH [--suite-root PATH] [--tests-dir PATH] \\" >&2
  echo "       [--include-dir PATH] [--std FLAG] [--expected-fail FILE] [-t]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --tests-dir) TESTS_DIR=$2; shift 2 ;;
    --include-dir) INCLUDE_DIR=$2; shift 2 ;;
    --std) DEFAULT_STD=$2; shift 2 ;;
    --expected-fail) EXPECT_FAIL_FILE=$2; shift 2 ;;
    -t|--terminate-on-fail) TERMINATE=true; shift ;;
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

if [ ! -d "$SUITE_ROOT/$TESTS_DIR" ]; then
  echo "tests directory not found: $SUITE_ROOT/$TESTS_DIR" >&2
  exit 1
fi

# Tests named in the expected-fail file (as `pass/NAME` or `fail/NAME`) are
# allowed to fail.  One that passes fails the suite, so that fixing a test
# forces its entry to be removed and the list cannot quietly go stale.  The
# list is one delimited string because the bash macOS ships has no
# associative arrays.
EXPECTED_FAILS="|"
if [ -n "$EXPECT_FAIL_FILE" ]; then
  EXPECT_FAIL_FILE=$(resolve_runfile "$EXPECT_FAIL_FILE")
  if [ ! -f "$EXPECT_FAIL_FILE" ]; then
    echo "expected-fail file not found: $EXPECT_FAIL_FILE" >&2
    exit 2
  fi
  while IFS= read -r line || [ -n "$line" ]; do
    line="${line%%#*}"
    line=$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    if [ -n "$line" ]; then
      EXPECTED_FAILS="${EXPECTED_FAILS}${line}|"
    fi
  done < "$EXPECT_FAIL_FILE"
fi

is_expected_fail() {
  case "$EXPECTED_FAILS" in
    *"|$1|"*) return 0 ;;
  esac
  return 1
}

work=$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-testsuite.XXXXXX")
trap 'rm -rf "$work"' EXIT

read_run_args() {
  local file=$1
  local line
  RUN_ARGS=()
  while IFS= read -r line; do
    case "$line" in
      "// RUN:"*)
        # Test metadata is intentionally shell-like to keep the suite simple.
        eval "RUN_ARGS+=(${line#// RUN:})"
        ;;
    esac
  done < "$file"
}

print_expected_diags() {
  local file=$1
  local line
  while IFS= read -r line; do
    case "$line" in
      "// EXPECT:"*)
        printf '%s\n' "${line#// EXPECT: }"
        ;;
    esac
  done < "$file"
}

run_compile() {
  local src=$1
  local log=$2
  local out=$3
  read_run_args "$src"
  local args=(-target pcode -S "$DEFAULT_STD" -isystem "$INCLUDE_DIR")
  args+=("${RUN_ARGS[@]}")
  if [ -n "${DAVECC_CONSTEXPR_EVAL:-}" ]; then
    args+=("-fconstexpr-eval=${DAVECC_CONSTEXPR_EVAL}")
  fi
  python3 -c '
import os, signal, subprocess, sys
cmd = sys.argv[1:]
proc = subprocess.Popen(
    cmd,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    start_new_session=True,
)
try:
    out, _ = proc.communicate(timeout=30)
    sys.stdout.buffer.write(out or b"")
    rc = proc.returncode or 0
    if rc < 0:
        sys.stdout.write("CRASH: compiler terminated by signal %d\n" % -rc)
        raise SystemExit(125)
    raise SystemExit(rc)
except subprocess.TimeoutExpired:
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except OSError:
        proc.kill()
    sys.stdout.write("TIMEOUT\n")
    raise SystemExit(124)
' "$DAVECC" "${args[@]}" "$src" -o "$out" >"$log" 2>&1
  return $?
}

# The compiler must terminate normally even on invalid input, so a crash or a
# hang is a failure in both directories.  Tests under fail/ only look for
# expected diagnostics, which a crash can emit before dying.
compiler_aborted() {
  [ "$1" -eq 124 ] || [ "$1" -eq 125 ]
}

pass=0
fail=0
compile_fail=0
diagnostic_fail=0
known_fail=0
unexpected_pass=0

# The single place a test's outcome is reconciled with the expected-fail list.
# `kind` is the failure counter to bump (compile or diagnostic).
record_fail() {
  local name=$1 detail=$2 kind=$3 log=$4
  if is_expected_fail "$name"; then
    echo "known-fail $name${detail:+ ($detail)}"
    known_fail=$((known_fail + 1))
    return
  fi
  echo "FAIL $name${detail:+ ($detail)}"
  if [ "$kind" = compile ]; then
    compile_fail=$((compile_fail + 1))
  else
    diagnostic_fail=$((diagnostic_fail + 1))
  fi
  fail=$((fail + 1))
  if [ -n "$log" ]; then
    sed 's/^/  /' "$log" | head -20
  fi
  if $TERMINATE; then exit 1; fi
}

record_pass() {
  local name=$1
  if is_expected_fail "$name"; then
    echo "UNEXPECTED PASS $name (now passes: remove it from the expected-fail list)"
    unexpected_pass=$((unexpected_pass + 1))
    return
  fi
  echo "ok $name"
  pass=$((pass + 1))
}

echo "=== cxx_testsuite compile: tests=$TESTS_DIR ==="
echo "davecc=$DAVECC"
echo "default_std=$DEFAULT_STD"
echo

for src in "$SUITE_ROOT/$TESTS_DIR"/pass/*.cpp; do
  [ -e "$src" ] || continue
  base=$(basename "$src")
  log="$work/$base.log"
  out="$work/$base.s"
  status=0
  run_compile "$src" "$log" "$out" || status=$?
  if [ "$status" -ne 0 ] || grep -q "error:" "$log"; then
    record_fail "pass/$base" "" compile "$log"
    continue
  fi
  record_pass "pass/$base"
done

for src in "$SUITE_ROOT/$TESTS_DIR"/fail/*.cpp; do
  [ -e "$src" ] || continue
  base=$(basename "$src")
  log="$work/$base.log"
  out="$work/$base.s"
  status=0
  run_compile "$src" "$log" "$out" || status=$?
  if compiler_aborted "$status"; then
    record_fail "fail/$base" "compiler did not exit normally" compile "$log"
    continue
  fi
  if [ "$status" -eq 0 ] && ! grep -q "error:" "$log"; then
    record_fail "fail/$base" "expected diagnostic" diagnostic ""
    continue
  fi
  missing=""
  while IFS= read -r expected; do
    if ! grep -Fq "$expected" "$log"; then
      missing=$expected
      break
    fi
  done < <(print_expected_diags "$src")
  if [ -n "$missing" ]; then
    record_fail "fail/$base" "missing diagnostic: $missing" diagnostic "$log"
    continue
  fi
  record_pass "fail/$base"
done

echo
echo "=== summary cxx_testsuite compile ==="
if [ -n "$EXPECT_FAIL_FILE" ]; then
  echo "pass=$pass fail=$fail known-fail=$known_fail" \
       "unexpected-pass=$unexpected_pass"
else
  echo "pass=$pass fail=$fail"
fi
echo "  compile_fail=$compile_fail diagnostic_fail=$diagnostic_fail"

if [ "$fail" -ne 0 ] || [ "$unexpected_pass" -ne 0 ]; then
  exit 1
fi
