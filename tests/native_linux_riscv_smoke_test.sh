#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
STARTUP="$ROOT/$3"
SOURCE="$ROOT/$4"
MODE="$5"
COLIMA="${COLIMA:-/opt/homebrew/bin/colima}"
COLIMA_PROFILE="${COLIMA_PROFILE:-default}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-linux-riscv.XXXXXX")"
REMOTE="/tmp/davecc-native-riscv-${RANDOM}-$$"

colima() {
  "$COLIMA" -p "$COLIMA_PROFILE" "$@"
}

cleanup() {
  rm -rf "$WORK"
  colima ssh -- rm -f "$REMOTE" "$REMOTE.dtor" >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -x "$COLIMA" ]] || ! colima ssh -- true >/dev/null 2>&1; then
  echo "the AArch64 Colima profile '$COLIMA_PROFILE' must be running" >&2
  exit 1
fi
if [[ "$(colima ssh -- uname -m)" != "aarch64" ]]; then
  echo "RV64 tests require an AArch64 Colima guest" >&2
  exit 1
fi
if ! colima ssh -- test -r /proc/sys/fs/binfmt_misc/qemu-riscv64; then
  echo "RV64 tests require qemu-riscv64 binfmt support in Colima" >&2
  exit 1
fi

LIBDIR="$WORK/lib"
mkdir -p "$LIBDIR"
cp "$LIBC" "$LIBDIR/libcriscv_linux.a"
cp "$STARTUP" "$LIBDIR/riscv_linux_start.o"

LANGUAGE_FLAGS=()
if [[ "$MODE" == "full" ]]; then
  LANGUAGE_FLAGS=(-std=c++20)
fi
DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" "$DAVECC" \
  -target riscv-unknown-linux-davecc "${LANGUAGE_FLAGS[@]}" \
  "$SOURCE" -o "$WORK/program"

base64 <"$WORK/program" |
  colima ssh -- sh -lc "base64 -d > '$REMOTE' && chmod 0755 '$REMOTE'"
if [[ "$MODE" == "full" ]]; then
  colima ssh -- "$REMOTE" "$REMOTE.dtor"
  if [[ "$(colima ssh -- sh -lc "cat '$REMOTE.dtor'")" != \
        "destructor-ran" ]]; then
    echo "RV64 native global destructor did not run" >&2
    exit 1
  fi
else
  colima ssh -- "$REMOTE"
fi
