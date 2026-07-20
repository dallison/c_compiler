#!/usr/bin/env bash

set -euo pipefail

davecc=$1
riscvdasm=$2
aarch64dasm=$3
armdasm=$4
x86_64dasm=$5
elfdump=$6

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cat >"$work/smoke.c" <<'EOF'
extern int printf(const char*, ...);

int add(int a, int b) {
  return a + b;
}

const char* message(void) {
  return "hello";
}

int main(void) {
  printf("hello");
  return add(1, 2);
}
EOF

check_target() {
  local target=$1
  local tool=$2
  shift 2
  local obj="$work/$target.o"
  local out="$work/$target.dis"

  "$davecc" -target "$target" -O0 -c "$work/smoke.c" -o "$obj"
  "$tool" "$obj" >"$out"
  "$elfdump" -c "$obj" >"$work/$target.elfdump"
  for expected in "$@"; do
    if ! grep -E "$expected" "$out" >/dev/null; then
      echo "missing expected disassembly for $target: $expected" >&2
      sed -n '1,80p' "$out" >&2
      return 1
    fi
    if ! grep -E "$expected" "$work/$target.elfdump" >/dev/null; then
      echo "missing expected elfdump disassembly for $target: $expected" >&2
      sed -n '1,80p' "$work/$target.elfdump" >&2
      return 1
    fi
  done
}

check_target riscv "$riscvdasm" '^add:' 'add|addi'
check_target aarch64 "$aarch64dasm" '^add:' 'stp' 'adr' 'printf' ' ret$'
check_target arm "$armdasm" '^add:' 'add|bl|bx' 'movw' 'movt'
check_target x86_64 "$x86_64dasm" '^add:' 'mov|add|call|ret'

obj="$work/65c02.o"
"$davecc" -target 65c02 -O0 -c "$work/smoke.c" -o "$obj"
"$elfdump" -c "$obj" >"$work/65c02.elfdump"
for expected in '^add:' 'lda|sta|adc' 'jsr' 'rts'; do
  if ! grep -E "$expected" "$work/65c02.elfdump" >/dev/null; then
    echo "missing expected elfdump disassembly for 65c02: $expected" >&2
    sed -n '1,80p' "$work/65c02.elfdump" >&2
    exit 1
  fi
done

cat >"$work/zero-page.s" <<'EOF'
  .text
zero_page_comments:
  sta 0x9
  lda 0x82
  lda (0x8), y
  rts
EOF

zp_obj="$work/zero-page.o"
"$davecc" -target 65c02 -c "$work/zero-page.s" -o "$zp_obj"
"$elfdump" -c "$zp_obj" >"$work/zero-page.elfdump"
for expected in \
    'sta 0x09.*// __i0 \+ 1' \
    'lda 0x82.*// __mem_src' \
    'lda \(0x08\),y.*// __i0'; do
  if ! grep -E "$expected" "$work/zero-page.elfdump" >/dev/null; then
    echo "missing 65c02 zero-page register comment: $expected" >&2
    sed -n '1,80p' "$work/zero-page.elfdump" >&2
    exit 1
  fi
done

linked="$work/aarch64"
"$davecc" -target aarch64 "$work/smoke.c" -o "$linked"
"$elfdump" -c "$linked" >"$work/aarch64-linked.elfdump"
for symbol in add message main; do
  count=$(grep -c "^$symbol:$" "$work/aarch64-linked.elfdump" || true)
  if [[ "$count" -ne 1 ]]; then
    echo "expected one label for $symbol, found $count" >&2
    exit 1
  fi
done
