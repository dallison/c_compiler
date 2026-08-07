#!/usr/bin/env bash
set -euo pipefail

davecc=$1
src="${TEST_SRCDIR:-.}/${TEST_WORKSPACE:-_main}/tests/bit_intrinsics_codegen.cpp"
if [[ ! -f "$src" ]]; then
  src="tests/bit_intrinsics_codegen.cpp"
fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

compile() {
  local target=$1
  "$davecc" -std=c++20 -O1 -nostdinc -target "$target" -S "$src" \
    -o "$tmp/$target.s"
}

compile x86_64
grep -Eq '\brol[lq]?\b' "$tmp/x86_64.s"
grep -Eq '\bror[lq]?\b' "$tmp/x86_64.s"
grep -Eq '\bbsr[lq]\b' "$tmp/x86_64.s"
grep -Eq '\bbsf[lq]\b' "$tmp/x86_64.s"
! grep -Eq '\bpopcnt\b' "$tmp/x86_64.s"

compile aarch64
grep -Eq '\brorv\b' "$tmp/aarch64.s"
grep -Eq '\bclz\b' "$tmp/aarch64.s"
grep -Eq '\brbit\b' "$tmp/aarch64.s"

compile arm
grep -Eq '\bror\b' "$tmp/arm.s"
grep -Eq '\bclz\b' "$tmp/arm.s"
grep -Eq '\brbit\b' "$tmp/arm.s"

compile riscv
! grep -Eq '\b(call|popcnt|clz|ctz|rol|ror)\b' "$tmp/riscv.s"

compile 65c02
grep -Eq '\brol\b' "$tmp/65c02.s"
grep -Eq '\bror\b' "$tmp/65c02.s"
! grep -Eq '\bjsr[[:space:]]+__[su]mod' "$tmp/65c02.s"
