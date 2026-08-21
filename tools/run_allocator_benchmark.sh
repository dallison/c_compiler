#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
INTERPRETER="$ROOT/$3"
SOURCE="$ROOT/$4"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/allocator-benchmark.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

"$DAVECC" -target x86_64 -static -O1 -isystem "$ROOT/libc/include" \
  -I"$ROOT/libc" -Wl,-e -Wl,main "$SOURCE" "$LIBC" \
  -o "$WORK/allocator_benchmark"
"$INTERPRETER" -i "$WORK/allocator_benchmark"
