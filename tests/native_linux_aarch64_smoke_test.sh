#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
STARTUP="$ROOT/$3"
SOURCE="$ROOT/$4"
COLIMA="${COLIMA:-/opt/homebrew/bin/colima}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-linux-aarch64.XXXXXX")"
REMOTE="/tmp/davecc-native-linux-${RANDOM}-$$"

cleanup() {
  rm -rf "$WORK"
  "$COLIMA" ssh -- rm -f "$REMOTE" "$REMOTE.dtor" >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -x "$COLIMA" ]] || ! "$COLIMA" status >/dev/null 2>&1; then
  echo "the Colima Linux instance must be running" >&2
  exit 1
fi

LIBDIR="$WORK/lib"
mkdir -p "$LIBDIR"
cp "$LIBC" "$LIBDIR/libcaarch64_linux.a"
cp "$STARTUP" "$LIBDIR/aarch64_linux_start.o"

DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" "$DAVECC" \
  -target aarch64-unknown-linux-davecc -std=c++20 \
  "$SOURCE" -o "$WORK/native-linux-smoke"

base64 <"$WORK/native-linux-smoke" |
  "$COLIMA" ssh -- sh -lc "base64 -d > '$REMOTE' && chmod 0755 '$REMOTE'"
"$COLIMA" ssh -- "$REMOTE" "$REMOTE.dtor"
if [[ "$("$COLIMA" ssh -- sh -lc "cat '$REMOTE.dtor'")" != \
      "destructor-ran" ]]; then
  echo "native global destructor did not run" >&2
  exit 1
fi
