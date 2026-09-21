#!/usr/bin/env bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
STARTUP="$ROOT/$3"
SOURCE="$ROOT/$4"
COLIMA="${COLIMA:-/opt/homebrew/bin/colima}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-linux-aarch64-ld.XXXXXX")"
REMOTE="/tmp/davecc-native-ld-${RANDOM}-$$"

cleanup() {
  rm -rf "$WORK"
  "$COLIMA" ssh -- rm -rf "$REMOTE" >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -x "$COLIMA" ]] || ! "$COLIMA" status >/dev/null 2>&1; then
  echo "the Colima Linux instance must be running" >&2
  exit 1
fi
if [[ "$("$COLIMA" ssh -- uname -m)" != "aarch64" ]]; then
  echo "the Colima instance is not aarch64" >&2
  exit 1
fi
if ! "$COLIMA" ssh -- sh -lc "command -v ld >/dev/null"; then
  echo "Colima has no native ld" >&2
  exit 1
fi

LIBDIR="$WORK/lib"
mkdir -p "$LIBDIR"
cp "$LIBC" "$LIBDIR/libcaarch64_linux.a"
cp "$STARTUP" "$LIBDIR/aarch64_linux_start.o"

cat >"$WORK/colima-ld" <<EOF
#!/bin/bash
set -euo pipefail
COLIMA="${COLIMA}"
REMOTE="${REMOTE}"
copy_to_remote() {
  local src=\$1 dest=\$2
  base64 < "\$src" | "\$COLIMA" ssh -- sh -lc "base64 -d > '\$dest'"
}
"\$COLIMA" ssh -- mkdir -p "\$REMOTE"
out=""
remote_args=()
idx=0
args=("\$@")
i=0
while [[ \$i -lt \${#args[@]} ]]; do
  arg="\${args[\$i]}"
  if [[ "\$arg" == "-o" ]]; then
    i=\$((i + 1))
    out="\${args[\$i]}"
    remote_args+=(-o "\$REMOTE/out")
  elif [[ "\$arg" == "-T" || "\$arg" == "--script" ]]; then
    i=\$((i + 1))
    dest="\$REMOTE/script\$idx"
    idx=\$((idx + 1))
    copy_to_remote "\${args[\$i]}" "\$dest"
    remote_args+=(-T "\$dest")
  elif [[ -f "\$arg" ]]; then
    dest="\$REMOTE/f\$idx-\$(basename "\$arg")"
    idx=\$((idx + 1))
    copy_to_remote "\$arg" "\$dest"
    remote_args+=("\$dest")
  else
    remote_args+=("\$arg")
  fi
  i=\$((i + 1))
done
if [[ -z "\$out" ]]; then
  echo "colima-ld: missing -o" >&2
  exit 1
fi
if ! "\$COLIMA" ssh -- ld "\${remote_args[@]}" 2>"$WORK/ld.err"; then
  cat "$WORK/ld.err" >&2
  exit 1
fi
if grep -F "no .eh_frame_hdr table will be created" "$WORK/ld.err" >/dev/null; then
  cat "$WORK/ld.err" >&2
  echo "colima-ld: GNU ld rejected .eh_frame" >&2
  exit 1
fi
"\$COLIMA" ssh -- sh -lc "base64 < '\$REMOTE/out'" | base64 -d > "\$out"
chmod +x "\$out"
EOF
chmod +x "$WORK/colima-ld"

DAVECC_INCLUDE_DIR="$ROOT/libc/include" DAVECC_LIB_DIR="$LIBDIR" "$DAVECC" \
  -target aarch64-unknown-linux-davecc -std=c++20 \
  -fuse-ld="$WORK/colima-ld" \
  "$SOURCE" -o "$WORK/native-linux-smoke"

base64 <"$WORK/native-linux-smoke" |
  "$COLIMA" ssh -- sh -lc "base64 -d > '$REMOTE/program' && chmod 0755 '$REMOTE/program'"
"$COLIMA" ssh -- "$REMOTE/program" "$REMOTE/program.dtor"
if [[ "$("$COLIMA" ssh -- sh -lc "cat '$REMOTE/program.dtor'")" != \
      "destructor-ran" ]]; then
  echo "native-ld global destructor did not run" >&2
  exit 1
fi
