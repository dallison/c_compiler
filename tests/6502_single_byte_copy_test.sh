#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="${TEST_TMPDIR:-/tmp}/6502-single-byte-copy"
mkdir -p "$WORK"

function_body() {
  local name="$1"
  local assembly="$2"
  awk -v start="$name:" -v finish=".func_end_$name:" \
    '$0 == start {inside=1; next} $0 == finish {inside=0} inside' "$assembly"
}

for optimization in O0 O1 Os; do
  for target in 6502 65c02; do
    assembly="$WORK/$target-$optimization.s"
    "$DAVECC" -target "$target" "-$optimization" -S -nostdinc \
      "$ROOT/tests/6502_single_byte_copy_cases.c" -o "$assembly"

    copy_byte_body=$(function_body copy_byte "$assembly")
    if grep -q '__copymem1' <<<"$copy_byte_body"; then
      echo "$target -$optimization: one-byte copy still calls __copymem1" >&2
      exit 1
    fi

    copy_twice_body=$(function_body copy_byte_twice "$assembly")
    indirect_loads=$(
      grep -Ec 'lda[[:space:]]+\(__i[0-9]+\)(,[[:space:]]*Y)?' \
        <<<"$copy_twice_body" || true
    )
    indirect_stores=$(
      grep -Ec 'sta[[:space:]]+\(__i[0-9]+\)(,[[:space:]]*Y)?' \
        <<<"$copy_twice_body" || true
    )
    if [[ "$indirect_loads" -ne 1 || "$indirect_stores" -ne 1 ]]; then
      echo "$target -$optimization: repeated copy was not deduplicated" >&2
      exit 1
    fi

    zero_byte_body=$(function_body zero_byte "$assembly")
    if [[ "$target" != "6502" || "$optimization" != "Os" ]] &&
       grep -q '__zeromem1' <<<"$zero_byte_body"; then
      echo "$target -$optimization: one-byte zero still calls __zeromem1" >&2
      exit 1
    fi

    copy_pair_body=$(function_body copy_pair "$assembly")
    zero_pair_body=$(function_body zero_pair "$assembly")
    if [[ "$optimization" == "O1" ]]; then
      if grep -q '__copymem1' <<<"$copy_pair_body" ||
         grep -q '__zeromem1' <<<"$zero_pair_body"; then
        echo "$target -$optimization: two-byte memory helper was not inlined" >&2
        exit 1
      fi
    else
      if ! grep -q '__copymem1' <<<"$copy_pair_body" ||
         ! grep -q '__zeromem1' <<<"$zero_pair_body"; then
        echo "$target -$optimization: size-oriented two-byte helper was lost" >&2
        exit 1
      fi
    fi

    if [[ "$target" == "6502" ]] &&
       grep -Eq '\([^)]*\)([^,]|$)' <<<"$assembly"; then
      echo "$target -$optimization: emitted 65C02-only indirect addressing" >&2
      exit 1
    fi
  done
done

echo "ok 6502 small copies and zeroes use balanced inline sequences"
