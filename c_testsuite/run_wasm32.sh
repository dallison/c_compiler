#!/bin/bash
#
# Run the c_testsuite single-exec tests against the wasm32 backend.
#
# The c_testsuite single-exec tests for wasm32.  Also available as
# `bazel test //c_testsuite:single_exec_wasm32` (needs host wasmtime).
#
# Each test is linked as a WASI command and run at both optimization levels,
# because at -O1 a variable nothing points at moves from the shadow frame into
# a wasm local and the two are not the same code.
#
# Usage: run_wasm32.sh [extra run_single_exec.sh arguments...]

set -u

root=$(cd "$(dirname "$0")/.." && pwd)

for tool in wasmtime; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "run_wasm32.sh: $tool is not installed" >&2
    exit 2
  fi
done

davecc="$root/bazel-bin/davecc"
libc="$root/bazel-bin/libc/libcwasm32.a"
runner="$root/tools/wasm32run.sh"
for artifact in "$davecc" "$libc" "$runner"; do
  if [ ! -e "$artifact" ]; then
    echo "run_wasm32.sh: $artifact is missing; run" >&2
    echo "  bazel build //:davecc //:libc_wasm32" >&2
    exit 2
  fi
done

status=0
for opt in -O0 -O1; do
  echo "################ wasm32 $opt ################"
  "$root/c_testsuite/run_single_exec.sh" \
    --davecc "$davecc" \
    --target wasm32 \
    --libc "$libc" \
    --interpreter "$runner" \
    --suite-root "$root/c_testsuite" \
    --skip "$root/c_testsuite/skip/davecc-wasm32.skip" \
    --compile-arg -target --compile-arg wasm32 \
    --compile-arg "$opt" \
    --compile-arg -isystem --compile-arg "$root/libc/include" \
    --timeout 60 \
    "$@" || status=1
done

exit $status
