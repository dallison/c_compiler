#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
DAVECC_SOURCE="$ROOT/$2"
FOREIGN_SOURCE="$ROOT/$3"
TARGET="$4"
FOREIGN_CXX="$5"
COLIMA_PROFILE="$6"
GUEST_ARCH="$7"
COLIMA="${COLIMA:-/opt/homebrew/bin/colima}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-foreign-eh.XXXXXX")"
REMOTE="/tmp/davecc-foreign-eh-${RANDOM}-$$"

colima() {
  "$COLIMA" -p "$COLIMA_PROFILE" "$@"
}

cleanup() {
  rm -rf "$WORK"
  colima ssh -- rm -rf "$REMOTE" >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -x "$COLIMA" ]] || ! colima ssh -- true >/dev/null 2>&1; then
  echo "the Colima profile '$COLIMA_PROFILE' must be running" >&2
  exit 1
fi
if [[ "$(colima ssh -- uname -m)" != "$GUEST_ARCH" ]]; then
  echo "the Colima profile '$COLIMA_PROFILE' must be $GUEST_ARCH" >&2
  exit 1
fi
if ! colima ssh -- sh -lc "command -v '$FOREIGN_CXX' >/dev/null"; then
  echo "missing foreign compiler in Colima: $FOREIGN_CXX" >&2
  exit 1
fi

"$DAVECC" -target "$TARGET-unknown-linux-davecc" -std=c++20 -O1 \
  -fexceptions -c "$DAVECC_SOURCE" -o "$WORK/davecc.o"

colima ssh -- mkdir -p "$REMOTE"
base64 <"$WORK/davecc.o" |
  colima ssh -- sh -lc "base64 -d > '$REMOTE/davecc.o'"
base64 <"$FOREIGN_SOURCE" |
  colima ssh -- sh -lc "base64 -d > '$REMOTE/foreign.cpp'"

colima ssh -- sh -lc \
  "cd '$REMOTE' && '$FOREIGN_CXX' -std=c++20 -O1 -fexceptions \
     -fomit-frame-pointer -no-pie foreign.cpp davecc.o -o interop"
colima ssh -- "$REMOTE/interop"
