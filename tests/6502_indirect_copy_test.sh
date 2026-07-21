#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="${TEST_TMPDIR:-/tmp}/6502-indirect-copy"
mkdir -p "$WORK"

cat >"$WORK/copy.c" <<'SRC'
unsigned long load_four(const unsigned long* address) {
  return *address;
}

void store_four(unsigned long* address, unsigned long value) {
  *address = value;
}

unsigned long long load_eight(const unsigned long long* address) {
  return *address;
}

void store_eight(unsigned long long* address, unsigned long long value) {
  *address = value;
}
SRC

for target in 6502 65c02; do
  "$DAVECC" -target "$target" -nostdinc -S "$WORK/copy.c" \
    -o "$WORK/copy-$target.s"

  if ! grep -q "jsr[[:space:]]*__load_indirect4" \
      "$WORK/copy-$target.s"; then
    echo "FAIL $target four-byte indirect load was not factored" >&2
    exit 1
  fi
  if ! grep -q "jsr[[:space:]]*__store_indirect4" \
      "$WORK/copy-$target.s"; then
    echo "FAIL $target four-byte indirect store was not factored" >&2
    exit 1
  fi
  if ! grep -q "jsr[[:space:]]*__load_indirect8" \
      "$WORK/copy-$target.s"; then
    echo "FAIL $target eight-byte indirect load was not factored" >&2
    exit 1
  fi
  if ! grep -q "jsr[[:space:]]*__store_indirect8" \
      "$WORK/copy-$target.s"; then
    echo "FAIL $target eight-byte indirect store was not factored" >&2
    exit 1
  fi
done

echo "ok four- and eight-byte indirect copies use compact helpers"
