#!/bin/sh
set -eu

root="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
davecc="$root/$1"
aarch64asm="$root/$2"
work="$(mktemp -d "${TMPDIR:-/tmp}/aarch64-direct-object.XXXXXX")"
trap 'rm -rf "$work"' EXIT

cat >"$work/input.cpp" <<'EOF'
template <int N>
__attribute__((noinline)) int value() {
  return N;
}

int main() {
  return value<0>() + value<1>() + value<65535>();
}
EOF

"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -c "$work/input.cpp" \
  -o "$work/direct.o"
"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -S "$work/input.cpp" \
  -o "$work/reference.s"
"$aarch64asm" "$work/reference.s" -o "$work/reference.o"

cmp "$work/reference.o" "$work/direct.o"
