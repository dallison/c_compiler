#!/usr/bin/env bash
# Casting a pointer difference from extern T[] to a narrower unsigned type
# must not emit illegal AArch64 UBFX (W-form width 64 from a zero-size IR type).
set -euo pipefail

DAVECC=${1:?davecc}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cat >"$tmp/diff.c" <<'EOF'
extern char start[];
extern char end[];
unsigned diff_len(void) {
  return (unsigned)(end - start);
}
EOF

"$DAVECC" -target aarch64-unknown-daveos-davecc -S -O2 \
  "$tmp/diff.c" -o "$tmp/diff.s"

if grep -E 'ubfx[[:space:]].*#64' "$tmp/diff.s" >/dev/null; then
  echo "illegal ubfx width 64 in:" >&2
  cat "$tmp/diff.s" >&2
  exit 1
fi

# Result must be the low 32 bits of the pointer subtraction.
grep -q 'sub x' "$tmp/diff.s" || {
  echo "expected pointer sub in:" >&2
  cat "$tmp/diff.s" >&2
  exit 1
}

echo ok
