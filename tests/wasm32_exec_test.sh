#!/bin/bash
# Bazel wrapper around wasm32_testsuite/run_exec_tests.sh.
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "usage: $0 <davecc> <archivist> <libc_wasm32>" >&2
  exit 2
fi

davecc=$1
archivist=$2
libc=$3

if [ -f "wasm32_testsuite/run_exec_tests.sh" ]; then
  :
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

export DAVECC_INCLUDE_DIR="${DAVECC_INCLUDE_DIR:-$PWD/libc/include}"
export DAVECC_LIB_DIR="${DAVECC_LIB_DIR:-$(dirname "$libc")}"
export ARCHIVIST=$archivist

exec bash wasm32_testsuite/run_exec_tests.sh "$davecc"
