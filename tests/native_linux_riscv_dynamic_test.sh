#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBDAVECC="$ROOT/$2"
SOURCE="$ROOT/$3"
MODE="$4"
COLIMA="${COLIMA:-/opt/homebrew/bin/colima}"
COLIMA_PROFILE="${COLIMA_PROFILE:-default}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-linux-riscv-dynamic.XXXXXX")"
REMOTE="/tmp/davecc-native-riscv-dynamic-${RANDOM}-$$"

colima() {
  "$COLIMA" -p "$COLIMA_PROFILE" "$@"
}

cleanup() {
  rm -rf "$WORK"
  colima ssh -- rm -rf "$REMOTE" >/dev/null 2>&1 || true
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
if ! colima ssh -- test -x /lib/ld-linux-riscv64-lp64d.so.1; then
  echo "RV64 dynamic tests require /lib/ld-linux-riscv64-lp64d.so.1" >&2
  exit 1
fi

RUNTIME_DIR="$(dirname "$LIBDAVECC")"
LANGUAGE_FLAGS=()
if [[ "$MODE" == "full" ]]; then
  LANGUAGE_FLAGS=(-std=c++20)
fi
DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$RUNTIME_DIR" \
  "$DAVECC" -target riscv-unknown-linux-davecc -dynamic \
  "${LANGUAGE_FLAGS[@]}" \
  "$SOURCE" -o "$WORK/program"

colima ssh -- mkdir -p "$REMOTE"
base64 <"$WORK/program" |
  colima ssh -- sh -lc "base64 -d > '$REMOTE/program' && chmod 0755 '$REMOTE/program'"
base64 <"$LIBDAVECC" |
  colima ssh -- sh -lc "base64 -d > '$REMOTE/libdavecc.so.1'"

colima ssh -- sh -lc "
set -eu
readelf -W -l '$REMOTE/program' |
  grep -Fq 'Requesting program interpreter: /lib/ld-linux-riscv64-lp64d.so.1'
readelf -W -d '$REMOTE/program' | grep -Fq 'Shared library: [libdavecc.so.1]'
readelf -W -d '$REMOTE/program' | grep -Fq 'Library runpath: [\$ORIGIN]'
readelf -W -d '$REMOTE/program' | grep -Fq '(BIND_NOW)'
readelf -W -d '$REMOTE/program' | grep -Fq '(RELA)'
readelf -W -r '$REMOTE/program' | grep -Fq 'R_RISCV_64'
readelf -W -r '$REMOTE/program' | grep -Fq 'R_RISCV_JUMP_SLOT'
if readelf -W -d '$REMOTE/program' |
    grep -Eq '\\((PREINIT_ARRAY|INIT_ARRAY|FINI_ARRAY)\\)'; then
  echo 'application lifecycle arrays were exposed to ld.so' >&2
  exit 1
fi
readelf -W -d '$REMOTE/libdavecc.so.1' |
  grep -Fq 'Library soname: [libdavecc.so.1]'
if readelf -W -l '$REMOTE/libdavecc.so.1' |
    grep -Eq '^[[:space:]]*TLS[[:space:]]'; then
  echo 'libdavecc.so.1 unexpectedly contains PT_TLS' >&2
  exit 1
fi
"

if [[ "$MODE" == "full" ]]; then
  colima ssh -- "$REMOTE/program" "$REMOTE/destructor-ran"
  if [[ "$(colima ssh -- sh -lc "cat '$REMOTE/destructor-ran'")" != \
        "destructor-ran" ]]; then
    echo "RV64 dynamic native global destructor did not run" >&2
    exit 1
  fi
else
  colima ssh -- "$REMOTE/program"
fi
