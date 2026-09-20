#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 davecc libc_wasm32 [wasm32run]" >&2
  exit 2
fi

if [[ -f "tests/regression/long_double_lib_test.c" ]]; then
  :
elif [[ -n "${TEST_SRCDIR:-}" && -n "${TEST_WORKSPACE:-}" ]]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

DAVECC="$1"
LIBC="$2"
RUNNER="${3:-}"
SOURCE="tests/regression/long_double_lib_test.c"
WORK="${TEST_TMPDIR:-/tmp}/long-double-lib-wasm32"
mkdir -p "$WORK"

if [[ -z "$RUNNER" ]]; then
  if [[ -x tools/wasm32run.sh ]]; then
    RUNNER=tools/wasm32run.sh
  elif command -v wasmtime >/dev/null 2>&1; then
    RUNNER="$(command -v wasmtime)"
  else
    echo "wasm32 long double lib test: wasmtime is not installed" >&2
    exit 2
  fi
fi

"$DAVECC" -target wasm32 -static -std=c99 \
  -isystem libc/include "$SOURCE" "$LIBC" \
  -o "$WORK/ldlib.wasm"

set +e
if [[ "$(basename "$RUNNER")" == "wasmtime" ]]; then
  "$RUNNER" run -W exceptions --dir "${PWD}::/" --dir /tmp::/tmp \
    "$WORK/ldlib.wasm"
  rc=$?
else
  "$RUNNER" "$WORK/ldlib.wasm"
  rc=$?
fi
set -e
if [[ "$rc" -ne 0 ]]; then
  echo "long double lib test (wasm32) returned $rc; expected 0" >&2
  exit 1
fi
