#!/bin/bash
# Run compile-only C++ tests for DaveCC.
set -uo pipefail

DAVECC=""
SUITE_ROOT=""
TESTS_DIR="tests/lexical"
INCLUDE_DIR=""
DEFAULT_STD="-std=c++20"
TERMINATE=false

usage() {
  echo "usage: $0 --davecc PATH [--suite-root PATH] [--tests-dir PATH] \\" >&2
  echo "       [--include-dir PATH] [--std FLAG] [-t]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --tests-dir) TESTS_DIR=$2; shift 2 ;;
    --include-dir) INCLUDE_DIR=$2; shift 2 ;;
    --std) DEFAULT_STD=$2; shift 2 ;;
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
    echo "FAIL pass/$base"
    compile_fail=$((compile_fail + 1))
    fail=$((fail + 1))
    sed 's/^/  /' "$log" | head -20
    if $TERMINATE; then exit 1; fi
    continue
  fi
  echo "ok pass/$base"
  pass=$((pass + 1))
done

for src in "$SUITE_ROOT/$TESTS_DIR"/fail/*.cpp; do
  [ -e "$src" ] || continue
  base=$(basename "$src")
  log="$work/$base.log"
  out="$work/$base.s"
  status=0
  run_compile "$src" "$log" "$out" || status=$?
  if compiler_aborted "$status"; then
    echo "FAIL fail/$base (compiler did not exit normally)"
    compile_fail=$((compile_fail + 1))
    fail=$((fail + 1))
    sed 's/^/  /' "$log" | head -20
    if $TERMINATE; then exit 1; fi
    continue
  fi
  if [ "$status" -eq 0 ] && ! grep -q "error:" "$log"; then
    echo "FAIL fail/$base (expected diagnostic)"
    diagnostic_fail=$((diagnostic_fail + 1))
    fail=$((fail + 1))
    if $TERMINATE; then exit 1; fi
    continue
  fi
  missing=0
  while IFS= read -r expected; do
    if ! grep -Fq "$expected" "$log"; then
      echo "FAIL fail/$base (missing diagnostic: $expected)"
      diagnostic_fail=$((diagnostic_fail + 1))
      fail=$((fail + 1))
      missing=1
      sed 's/^/  /' "$log" | head -20
      break
    fi
  done < <(print_expected_diags "$src")
  if [ "$missing" -ne 0 ]; then
    if $TERMINATE; then exit 1; fi
    continue
  fi
  echo "ok fail/$base"
  pass=$((pass + 1))
done

echo
echo "=== summary cxx_testsuite compile ==="
echo "pass=$pass fail=$fail"
echo "  compile_fail=$compile_fail diagnostic_fail=$diagnostic_fail"

if [ "$fail" -ne 0 ]; then
  exit 1
fi
