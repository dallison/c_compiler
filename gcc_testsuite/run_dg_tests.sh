#!/bin/bash
# Run external GCC DejaGNU frontend tests against davecc.
set -eo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
DAVECC=""
ROOT="${GCC_TEST_ROOT:-}"
EXTRA=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --root) ROOT=$2; shift 2 ;;
    *) EXTRA+=("$1"); shift ;;
  esac
done

if [ -z "$DAVECC" ]; then
  echo "usage: $0 --davecc PATH [--root gcc/testsuite] [run_dg_tests.py args...]" >&2
  exit 2
fi

if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  if [ -e "${TEST_SRCDIR}/${TEST_WORKSPACE}/${DAVECC}" ]; then
    DAVECC="${TEST_SRCDIR}/${TEST_WORKSPACE}/${DAVECC}"
  fi
fi

exec python3 "$SCRIPT_DIR/run_dg_tests.py" --davecc "$DAVECC" \
  ${ROOT:+--root "$ROOT"} "${EXTRA[@]}"
