#!/usr/bin/env bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
STARTUP="$ROOT/$3"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-ld-driver.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

LIBDIR="$WORK/lib"
mkdir -p "$LIBDIR"
cp "$LIBC" "$LIBDIR/libcaarch64_linux.a"
cp "$STARTUP" "$LIBDIR/aarch64_linux_start.o"

cat >"$WORK/program.c" <<'SRC'
int main(void) { return 0; }
SRC

if DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" \
    "$DAVECC" -target x86_64 -fuse-ld=ld "$WORK/program.c" -o "$WORK/bad" \
    >"$WORK/reject-interp.log" 2>&1; then
  echo "expected -fuse-ld to reject an interpreter-profile target" >&2
  cat "$WORK/reject-interp.log" >&2
  exit 1
fi
if ! grep -q 'Linux target' "$WORK/reject-interp.log"; then
  echo "missing Linux-target diagnostic" >&2
  cat "$WORK/reject-interp.log" >&2
  exit 1
fi

if [[ "$(uname -s)" == "Darwin" ]]; then
  if DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" \
      "$DAVECC" -target aarch64-unknown-linux-davecc -fuse-ld=ld \
      "$WORK/program.c" -o "$WORK/apple" \
      >"$WORK/reject-apple.log" 2>&1; then
    echo "expected -fuse-ld=ld to reject Apple ld" >&2
    cat "$WORK/reject-apple.log" >&2
    exit 1
  fi
  if ! grep -q 'Apple ld' "$WORK/reject-apple.log" &&
     ! grep -q "unable to find linker 'ld'" "$WORK/reject-apple.log"; then
    echo "expected Apple ld or missing-ld diagnostic" >&2
    cat "$WORK/reject-apple.log" >&2
    exit 1
  fi
fi

cat >"$WORK/record-ld" <<'EOF'
#!/bin/bash
set -euo pipefail
printf '%s\n' "$@" > "$RECORD_LD_LOG"
out=""
args=("$@")
for ((i = 0; i < ${#args[@]}; i++)); do
  if [[ "${args[$i]}" == "-o" && $((i + 1)) -lt ${#args[@]} ]]; then
    out="${args[$((i + 1))]}"
  fi
done
if [[ -z "$out" ]]; then
  out=a.out
fi
printf 'native-link-invoked\n' >"$out"
chmod +x "$out"
EOF
chmod +x "$WORK/record-ld"

export RECORD_LD_LOG="$WORK/ld-args.txt"
DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" \
  "$DAVECC" -target aarch64-unknown-linux-davecc \
  -fuse-ld="$WORK/record-ld" "$WORK/program.c" -o "$WORK/program"
if [[ "$(cat "$WORK/program")" != "native-link-invoked" ]]; then
  echo "native linker was not used to write the output" >&2
  cat "$RECORD_LD_LOG" >&2
  exit 1
fi
for expected in '^-static$' '^-e$' '^_start$' 'aarch64_linux_start.o$' \
                'libcaarch64_linux.a$'; do
  if ! grep -q "$expected" "$RECORD_LD_LOG"; then
    echo "missing native linker argument $expected" >&2
    cat "$RECORD_LD_LOG" >&2
    exit 1
  fi
done
if grep -q '^-dynamic$' "$RECORD_LD_LOG"; then
  echo "native linker argv still contains daveld -dynamic" >&2
  cat "$RECORD_LD_LOG" >&2
  exit 1
fi
