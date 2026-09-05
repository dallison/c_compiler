#!/bin/bash
# Run C++ execution tests with DaveCC + target libc + interpreter.
set -o pipefail

DAVECC=""
TARGET=""
LIBC=""
INTERPRETER=""
ROM=""
TESTS_DIR="tests/exec"
SUITE_ROOT=""
TIMEOUT=30
EXPECT_FAIL_FILE=""
declare -a INTERP_ARGS=()
declare -a COMPILE_ARGS=()

usage() {
  echo "usage: $0 --davecc PATH --target NAME --libc PATH --interpreter PATH [--rom PATH] [--tests-dir PATH] [--expected-fail PATH]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --target) TARGET=$2; shift 2 ;;
    --libc) LIBC=$2; shift 2 ;;
    --interpreter) INTERPRETER=$2; shift 2 ;;
    --rom) ROM=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --tests-dir) TESTS_DIR=$2; shift 2 ;;
    --timeout) TIMEOUT=$2; shift 2 ;;
    --interp-arg) INTERP_ARGS+=("$2"); shift 2 ;;
    --compile-arg) COMPILE_ARGS+=("$2"); shift 2 ;;
    --expected-fail) EXPECT_FAIL_FILE=$2; shift 2 ;;
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
if [ -n "$ROM" ]; then
  ROM=$(resolve_runfile "$ROM")
fi
if [ -n "$EXPECT_FAIL_FILE" ]; then
  EXPECT_FAIL_FILE=$(resolve_runfile "$EXPECT_FAIL_FILE")
fi
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
elif command -v python3 >/dev/null 2>&1; then
  # macOS does not provide timeout(1).
  TIMEOUT_CMD=(
    python3 -c
    'import subprocess, sys
try:
    result = subprocess.run(sys.argv[2:], timeout=float(sys.argv[1]))
    sys.exit(result.returncode if result.returncode >= 0 else 128 - result.returncode)
except subprocess.TimeoutExpired:
    sys.exit(124)'
    "$TIMEOUT"
  )
elif command -v perl >/dev/null 2>&1; then
  # Preserve the alarm across exec as a final portable fallback.
  TIMEOUT_CMD=(perl -e 'alarm shift @ARGV; exec @ARGV' "$TIMEOUT")
fi

work=$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-exec.XXXXXX")
trap 'rm -rf "$work"' EXIT

# Give the tests under test their own temporary directory.  A test that names a
# file under the system one picks the same name in every suite, and the suites
# for the different targets and optimization levels run at the same time.
mkdir -p "$work/tmp"
export TMPDIR="$work/tmp"

# Time-zone execution tests use a generated database so symlink aliases behave
# identically in a source checkout and Bazel's runfiles tree.
if [ -z "${DAVE_TZDIR:-}" ]; then
  TZIF_GENERATOR="$SUITE_ROOT/../tests/generate_tzif_fixtures.py"
  if [ -f "$TZIF_GENERATOR" ]; then
    python3 "$TZIF_GENERATOR" "$work/tzif"
    export DAVE_TZDIR="$work/tzif"
    export TZ="${TZ:-FixedOffset}"
  fi
fi

pass=0
fail=0
known_fail=0
unexpected_pass=0
flaky_seen=0
declare -a TEST_COMPILE_ARGS=()

# Tests named in the expected-fail file are allowed to fail; ones named there as
# "flaky NAME" are allowed either outcome, for a test that does not reach the
# same result twice running.  Both lists are held as one delimited string
# because the bash macOS ships has no associative arrays.
EXPECTED_FAILS="|"
FLAKY_TESTS="|"
if [ -n "$EXPECT_FAIL_FILE" ]; then
  if [ ! -f "$EXPECT_FAIL_FILE" ]; then
    echo "expected-fail file not found: $EXPECT_FAIL_FILE" >&2
    exit 2
  fi
  while IFS= read -r line || [ -n "$line" ]; do
    line="${line%%#*}"
    # Trim surrounding whitespace, keeping the separator between the optional
    # marker and the name.
    line=$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    case "$line" in
      "")
        ;;
      flaky[[:space:]]*)
        name=$(echo "${line#flaky}" | sed 's/^[[:space:]]*//')
        FLAKY_TESTS="${FLAKY_TESTS}${name}|"
        ;;
      *)
        EXPECTED_FAILS="${EXPECTED_FAILS}${line}|"
        ;;
    esac
  done < "$EXPECT_FAIL_FILE"
fi

is_expected_fail() {
  case "$EXPECTED_FAILS" in
    *"|$1|"*) return 0 ;;
  esac
  return 1
}

is_flaky() {
  case "$FLAKY_TESTS" in
    *"|$1|"*) return 0 ;;
  esac
  return 1
}

# The single place a test's outcome is reconciled with the expected-fail list.
# A test that fails when it was expected to is reported and forgiven; one that
# passes when it was expected to fail fails the suite, so that fixing a test
# forces its entry to be removed and the list cannot quietly go stale.
record_result() {
  local base=$1 outcome=$2 detail=$3 log=$4 log_lines=$5
  if is_flaky "$base"; then
    if [ "$outcome" = ok ]; then
      echo "flaky-ok $base"
    else
      echo "flaky-fail $base ($detail)"
    fi
    flaky_seen=$((flaky_seen + 1))
    return
  fi
  if [ "$outcome" = ok ]; then
    if is_expected_fail "$base"; then
      echo "UNEXPECTED PASS $base (now passes: remove it from the expected-fail list)"
      unexpected_pass=$((unexpected_pass + 1))
    else
      echo "ok $base"
      pass=$((pass + 1))
    fi
    return
  fi
  if is_expected_fail "$base"; then
    echo "known-fail $base ($detail)"
    known_fail=$((known_fail + 1))
    return
  fi
  echo "FAIL $base ($detail)"
  if [ -n "$log" ] && [ -f "$log" ]; then
    sed 's/^/  /' "$log" | head -"$log_lines"
  fi
  fail=$((fail + 1))
}

read_test_directives() {
  local file=$1
  local line
  EXPECT_EXIT=""
  TEST_STANDARD="-std=c++20"
  TEST_TARGETS=""
  TEST_COMPILE_ARGS=()
  while IFS= read -r line; do
    case "$line" in
      "// RUN:"*)
        local run_line="${line#// RUN: }"
        local -a run_args=()
        read -r -a run_args <<< "$run_line"
        local arg
        for arg in "${run_args[@]}"; do
          case "$arg" in
            -std=*) TEST_STANDARD="$arg" ;;
            *) TEST_COMPILE_ARGS+=("$arg") ;;
          esac
        done
        ;;
      "// EXPECT_EXIT:"*)
        EXPECT_EXIT="${line#// EXPECT_EXIT: }"
        ;;
      "// TARGETS:"*)
        TEST_TARGETS="${line#// TARGETS: }"
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
if [ -n "$ROM" ]; then
  echo "rom=$ROM"
fi
echo

for src in "$SUITE_ROOT/$TESTS_DIR"/*.cpp; do
  [ -e "$src" ] || continue
  base=$(basename "$src")
  read_test_directives "$src"
  if [ -n "$TEST_TARGETS" ]; then
    case " $TEST_TARGETS " in
      *" $TARGET "*) ;;
      *)
        echo "skip $base (target $TARGET)"
        continue
        ;;
    esac
  fi
  exp="${src}.expected"
  bin="$work/test.bin"
  out="$work/test.out"
  compile_cmd=("$DAVECC" -target "$TARGET" -static "$TEST_STANDARD"
               -isystem "$INCLUDE_DIR")
  if [ -n "${DAVECC_CONSTEXPR_EVAL:-}" ]; then
    compile_cmd+=("-fconstexpr-eval=${DAVECC_CONSTEXPR_EVAL}")
  fi
  compile_cmd+=("${TEST_COMPILE_ARGS[@]}" "${COMPILE_ARGS[@]}"
                "$src" "$LIBC" -o "$bin")
  if ! "${TIMEOUT_CMD[@]}" "${compile_cmd[@]}" \
      >"$work/compile.log" 2>&1; then
    record_result "$base" fail "compile" "$work/compile.log" 20
    continue
  fi

  run_cmd=("${TIMEOUT_CMD[@]}" "$INTERPRETER")
  if [ -n "$ROM" ]; then
    run_cmd+=(-rom "$ROM")
  fi
  run_cmd+=("${INTERP_ARGS[@]}" "$bin")
  "${run_cmd[@]}" >"$out" 2>"$work/run.err"
  run_status=$?
  if [ -n "$EXPECT_EXIT" ]; then
    if ! exit_status_matches "$EXPECT_EXIT" "$run_status"; then
      record_result "$base" fail \
          "run exit $run_status, expected $EXPECT_EXIT" "$work/run.err" 20
      continue
    fi
    if [ -f "$exp" ] && ! diff -u "$exp" "$out" >"$work/diff"; then
      record_result "$base" fail "output" "$work/diff" 40
      continue
    fi
    record_result "$base" ok
    continue
  fi
  if [ "$run_status" -ne 0 ]; then
    record_result "$base" fail "run exit $run_status" "$work/run.err" 20
    continue
  fi
  if [ -f "$exp" ] && ! diff -u "$exp" "$out" >"$work/diff"; then
    record_result "$base" fail "output" "$work/diff" 40
    continue
  fi
  record_result "$base" ok
done

echo
echo "=== summary cxx_testsuite exec ==="
if [ -n "$EXPECT_FAIL_FILE" ]; then
  echo "pass=$pass fail=$fail known-fail=$known_fail" \
       "unexpected-pass=$unexpected_pass flaky=$flaky_seen"
else
  echo "pass=$pass fail=$fail"
fi

if [ "$fail" -ne 0 ] || [ "$unexpected_pass" -ne 0 ]; then
  exit 1
fi
