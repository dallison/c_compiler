#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/6502-codegen-cleanup"
mkdir -p "$WORK"

for target in 6502 65c02; do
  assembly="$WORK/constants-$target.s"
  "$DAVECC" -target "$target" -O1 -S -std=c++20 -nostdinc \
    "$ROOT/tests/6502_constant_materialization_cases.cpp" -o "$assembly"

  overflow_body=$(
    awk '/^_ZN6BufferIhE8overflowEi:$/{inside=1; next} \
         /^\.func_end__ZN6BufferIhE8overflowEi:$/{inside=0} \
         inside' "$assembly"
  )
  store_count=$(
    grep -Ec 'sta[[:space:]]+__i0(\+1)?([[:space:]]|$)' <<<"$overflow_body"
  )
  if [[ "$store_count" -ne 2 ]]; then
    echo "$target: constant result was stored $store_count times, expected 2" >&2
    exit 1
  fi

  constructor_body=$(
    awk '/^_ZN6BufferIhEC1Ev:$/{inside=1; next} \
         /^\.func_end__ZN6BufferIhEC1Ev:$/{inside=0} \
         inside' "$assembly"
  )
  if grep -Eq 'ldy[[:space:]]+#4([[:space:]]|$)' <<<"$constructor_body"; then
    echo "$target: retained a load implied by CPY/BNE fallthrough" >&2
    exit 1
  fi

  symbol_push_body=$(
    awk '/^_Z21push_symbol_addressesv:$/{inside=1; next} \
         /^\.func_end__Z21push_symbol_addressesv:$/{inside=0} \
         inside' "$assembly"
  )
  symbol_push_count=$(
    grep -Ec 'jsr[[:space:]]+__pushxy([[:space:]]|$)' <<<"$symbol_push_body"
  )
  if [[ "$symbol_push_count" -ne 2 ]] ||
     grep -Eq 'sta[[:space:]]+__i[0-9]+' <<<"$symbol_push_body"; then
    echo "$target: symbolic addresses were copied through zero page" >&2
    exit 1
  fi

  call_widen_body=$(
    awk '/^_Z17widen_call_resultv:$/{inside=1; next} \
         /^\.func_end__Z17widen_call_resultv:$/{inside=0} \
         inside' "$assembly"
  )
  if grep -Eq 'lda[[:space:]]+__b2([[:space:]]|$)' <<<"$call_widen_body" ||
     ! awk '/sta[[:space:]]+__b2([[:space:]]|$)/ {
              getline
              found = $0 ~ /sta[[:space:]]+__i0([[:space:]]|$)/
            }
            END { exit !found }' <<<"$call_widen_body"; then
    echo "$target: retained a reload immediately after a zero-page store" >&2
    exit 1
  fi
done

cat >"$WORK/first.s" <<'ASM'
	.text
	.global first
	.type first, @function
first:
	nop
	rts
.first_end:
	.size first, .first_end-first
ASM

cat >"$WORK/second.s" <<'ASM'
	.text
	.global second
	.type second, @function
second:
	rts
.second_end:
	.size second, .second_end-second
ASM

"$DAVECC" -target 65c02 -nostdinc -c "$WORK/first.s" -o "$WORK/first.o"
"$DAVECC" -target 65c02 -nostdinc -c "$WORK/second.s" -o "$WORK/second.o"
"$DAVECC" -target 65c02 -nostdinc -nostdlib -static -Wl,-e -Wl,first \
  "$WORK/first.o" "$WORK/second.o" -o "$WORK/alignment.exe"

symbols=$("$ELFDUMP" -s "$WORK/alignment.exe")
read -r first_address first_size < <(
  awk '$NF == "first" {print $2, $3}' <<<"$symbols"
)
read -r second_address _ < <(
  awk '$NF == "second" {print $2, $3}' <<<"$symbols"
)
expected_second=$((16#$first_address + first_size))
if [[ $((16#$second_address)) -ne "$expected_second" ]]; then
  echo "65c02: linker inserted padding between adjacent text sections" >&2
  exit 1
fi

echo "ok 6502 constants and text sections contain no redundant bytes"
