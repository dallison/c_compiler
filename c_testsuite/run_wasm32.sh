#!/bin/bash
#
# Run the c_testsuite single-exec tests against the wasm32 backend.
#
# The other targets in this suite reach their programs through an interpreter
# built here in the tree, so Bazel can hand one to the test.  Wasm has no such
# interpreter of ours: the runtime is wasmtime, installed on the host, which
# is why this target is a script to be run by hand rather than a Bazel test
# alongside the others.
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
for artifact in "$davecc" "$libc"; do
  if [ ! -f "$artifact" ]; then
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
    --interpreter "$(command -v wasmtime)" \
    --interp-arg run \
    --suite-root "$root/c_testsuite" \
    --skip "$root/c_testsuite/skip/davecc-wasm32.skip" \
    --compile-arg -target --compile-arg wasm32 \
    --compile-arg "$opt" \
    --compile-arg -isystem --compile-arg "$root/libc/include" \
    --timeout 60 \
    "$@" || status=1
done

exit $status
