#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
STARTUP="$ROOT/$3"
SOURCE="$ROOT/$4"
COLIMA="${COLIMA:-/opt/homebrew/bin/colima}"
COLIMA_PROFILE="${COLIMA_PROFILE:-x86}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-linux-x86_64.XXXXXX")"
REMOTE="/tmp/davecc-native-linux-${RANDOM}-$$"

colima() {
  "$COLIMA" -p "$COLIMA_PROFILE" "$@"
}

cleanup() {
  rm -rf "$WORK"
  colima ssh -- rm -f "$REMOTE" "$REMOTE.dtor" >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -x "$COLIMA" ]] || ! colima ssh -- true >/dev/null 2>&1; then
  echo "the x86_64 Colima profile '$COLIMA_PROFILE' must be running" >&2
  exit 1
fi
if [[ "$(colima ssh -- uname -m)" != "x86_64" ]]; then
  echo "the Colima profile '$COLIMA_PROFILE' is not x86_64" >&2
  exit 1
fi

LIBDIR="$WORK/lib"
mkdir -p "$LIBDIR"
cp "$LIBC" "$LIBDIR/libcx86_64_linux.a"
cp "$STARTUP" "$LIBDIR/x86_64_linux_start.o"

DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" "$DAVECC" \
  -target x86_64-unknown-linux-davecc -std=c++20 \
  "$SOURCE" -o "$WORK/native-linux-smoke"

base64 <"$WORK/native-linux-smoke" |
  colima ssh -- sh -lc "base64 -d > '$REMOTE' && chmod 0755 '$REMOTE'"
colima ssh -- "$REMOTE" "$REMOTE.dtor"
if [[ "$(colima ssh -- sh -lc "cat '$REMOTE.dtor'")" != \
      "destructor-ran" ]]; then
  echo "native global destructor did not run" >&2
  exit 1
fi
