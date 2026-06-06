#!/bin/bash
# Run c_testsuite single-exec tests sequentially with DaveCC + interpreter.
# Intended to be invoked from Bazel with $(execpath ...) tool paths.
set -uo pipefail

DAVECC=""
TARGET=""
LIBC=""
INTERPRETER=""
ROM=""
SKIP_FILE=""
TESTS_DIR="tests/single-exec"
SUITE_ROOT=""
TIMEOUT=30
INTERP_ARGS=()
COMPILE_ARGS=()
START=""
END=""
TERMINATE=false

usage() {
  echo "usage: $0 --davecc PATH --target NAME --libc PATH --interpreter PATH \\" >&2
  echo "       [--rom PATH] [--skip PATH] [--suite-root PATH] [--timeout SEC] \\" >&2
  echo "       [--interp-arg ARG]... [--compile-arg ARG]... [--start N] [--end N] [-t]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --target) TARGET=$2; shift 2 ;;
    --libc) LIBC=$2; shift 2 ;;
    --interpreter) INTERPRETER=$2; shift 2 ;;
    --rom) ROM=$2; shift 2 ;;
    --skip) SKIP_FILE=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --tests-dir) TESTS_DIR=$2; shift 2 ;;
    --timeout) TIMEOUT=$2; shift 2 ;;
    --interp-arg) INTERP_ARGS+=("$2"); shift 2 ;;
    --compile-arg) COMPILE_ARGS+=("$2"); shift 2 ;;
    --start) START=$2; shift 2 ;;
    --end) END=$2; shift 2 ;;
    -t|--terminate-on-fail) TERMINATE=true; shift ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

if [ -z "$DAVECC" ] || [ -z "$TARGET" ] || [ -z "$INTERPRETER" ]; then
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

if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  DAVECC=$(resolve_runfile "$DAVECC")
  LIBC=$(resolve_runfile "$LIBC")
  INTERPRETER=$(resolve_runfile "$INTERPRETER")
  if [ -n "$ROM" ]; then
    ROM=$(resolve_runfile "$ROM")
  fi
  if [ -n "$SKIP_FILE" ]; then
    SKIP_FILE=$(resolve_runfile "$SKIP_FILE")
  fi
fi

if [ -z "$SUITE_ROOT" ] && [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  SUITE_ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE}/c_testsuite"
fi

# The libc headers (shipped via //:libc_headers) live at the workspace root,
# not under c_testsuite/, so a relative "-isystem libc/include" cannot find
# them once we cd into the suite directory.  Add an absolute include path.
if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  COMPILE_ARGS+=("-isystem" "${TEST_SRCDIR}/${TEST_WORKSPACE}/libc/include")
fi

if [ -n "$SUITE_ROOT" ]; then
  cd "$SUITE_ROOT"
fi

if [ ! -d "$TESTS_DIR" ]; then
  echo "tests directory not found: $TESTS_DIR" >&2
  exit 1
fi

if [ -z "$LIBC" ] || [ ! -f "$LIBC" ]; then
  echo "libc archive required and must exist: $LIBC" >&2
  exit 1
fi

if [ -n "$ROM" ] && [ ! -f "$ROM" ]; then
  echo "ROM file not found: $ROM" >&2
  exit 1
fi

if command -v gtimeout >/dev/null 2>&1; then
  TIMEOUT_CMD=(gtimeout "$TIMEOUT")
elif command -v timeout >/dev/null 2>&1; then
  TIMEOUT_CMD=(timeout "$TIMEOUT")
else
  TIMEOUT_CMD=()
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

declare -a SKIP_LIST=()
if [ -n "$SKIP_FILE" ] && [ -f "$SKIP_FILE" ]; then
  while IFS= read -r line; do
    case "$line" in
      tests/single-exec/*)
        SKIP_LIST+=("$(basename "$line")")
        ;;
    esac
  done < <(bash "$SKIP_FILE" 2>/dev/null || true)
fi

is_skipped() {
  local base=$1
  local s
  if [ "${#SKIP_LIST[@]}" -eq 0 ]; then
    return 1
  fi
  for s in "${SKIP_LIST[@]}"; do
    if [ "$s" = "$base" ]; then
      return 0
    fi
  done
  return 1
}

build_test_list() {
  TEST_LIST=()
  if [ -n "$START" ]; then
    local s=$START e=${END:-$START}
    local i
    i=$s
    while [ "$i" -le "$e" ]; do
      TEST_LIST+=("$(printf '%05d' "$i").c")
      i=$((i + 1))
    done
  else
    local t
    for t in "$TESTS_DIR"/*.c; do
      TEST_LIST+=("$(basename "$t")")
    done
  fi
}

build_test_list

pass=0
fail=0
skip=0
compile_fail=0
run_fail=0
output_fail=0
timeout_fail=0

echo "=== c_testsuite single-exec: target=$TARGET ==="
echo "davecc=$DAVECC"
echo "libc=$LIBC"
echo "interpreter=$INTERPRETER"
if [ -n "$ROM" ]; then
  echo "rom=$ROM"
fi
echo "tests=${#TEST_LIST[@]} (skip list: ${#SKIP_LIST[@]})"
echo

for base in "${TEST_LIST[@]}"; do
  t="$TESTS_DIR/$base"
  if [ ! -f "$t" ]; then
    continue
  fi
  if is_skipped "$base"; then
    echo "SKIP $base"
    skip=$((skip + 1))
    continue
  fi

  exp="${t}.expected"
  bin="$work/test.bin"
  out="$work/test.out"

  compile_cmd=("$DAVECC" "${COMPILE_ARGS[@]}" "$t" "$LIBC" -o "$bin")
  if ! "${compile_cmd[@]}" >"$work/compile.log" 2>&1; then
    echo "FAIL $base (compile)"
    compile_fail=$((compile_fail + 1))
    fail=$((fail + 1))
    sed 's/^/  /' "$work/compile.log" | head -20
    if $TERMINATE; then exit 1; fi
    continue
  fi

  run_cmd=()
  if [ "${#TIMEOUT_CMD[@]}" -gt 0 ]; then
    run_cmd=("${TIMEOUT_CMD[@]}")
  fi
  run_cmd+=("$INTERPRETER")
  if [ -n "$ROM" ]; then
    run_cmd+=(-rom "$ROM")
  fi
  if [ "${#INTERP_ARGS[@]}" -gt 0 ]; then
    run_cmd+=("${INTERP_ARGS[@]}")
  fi
  run_cmd+=("$bin")

  run_status=0
  if ! "${run_cmd[@]}" >"$out" 2>"$work/run.err"; then
    run_status=$?
  fi

  if [ "$run_status" -eq 124 ]; then
    echo "FAIL $base (timeout)"
    timeout_fail=$((timeout_fail + 1))
    fail=$((fail + 1))
    if $TERMINATE; then exit 1; fi
    continue
  fi

  if [ "$run_status" -ne 0 ]; then
    echo "FAIL $base (run exit $run_status)"
    run_fail=$((run_fail + 1))
    fail=$((fail + 1))
    if [ -s "$work/run.err" ]; then
      sed 's/^/  /' "$work/run.err" | head -10
    fi
    if $TERMINATE; then exit 1; fi
    continue
  fi

  if ! diff -u "$exp" "$out" >"$work/diff.log" 2>&1; then
    echo "FAIL $base (output)"
    output_fail=$((output_fail + 1))
    fail=$((fail + 1))
    sed 's/^/  /' "$work/diff.log" | head -15
    if $TERMINATE; then exit 1; fi
    continue
  fi

  echo "ok $base"
  pass=$((pass + 1))
done

echo
echo "=== summary target=$TARGET ==="
echo "pass=$pass fail=$fail skip=$skip"
echo "  compile_fail=$compile_fail run_fail=$run_fail output_fail=$output_fail timeout_fail=$timeout_fail"

if [ "$fail" -ne 0 ]; then
  exit 1
fi
