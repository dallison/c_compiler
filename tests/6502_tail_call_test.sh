#!/bin/bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc interpreter rom libc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
ROM="$ROOT/$3"
LIBC="$ROOT/$4"
WORK="${TEST_TMPDIR:-/tmp}/6502-tail-call"
mkdir -p "$WORK"

SOURCE="$WORK/tail_call.c"
cat >"$SOURCE" <<'SRC'
volatile unsigned char marker;

__attribute__((noinline)) unsigned short callee(const char* value) {
  return (unsigned short)value[0] + (unsigned short)value[1];
}

__attribute__((noinline)) unsigned short wrapper(const char* value) {
  return callee(value);
}

__attribute__((noinline)) unsigned short wrapper_with_work(const char* value) {
  marker++;
  return callee(value);
}

__attribute__((noinline)) unsigned short callee_number(unsigned short value) {
  return value;
}

__attribute__((noinline)) unsigned short changed_argument(unsigned short value) {
  return callee_number(value + 1);
}

__attribute__((noinline)) unsigned char callee_byte(unsigned char value) {
  return value;
}

__attribute__((noinline)) unsigned char changed_byte(unsigned char value) {
  return callee_byte((unsigned char)(value + 1));
}

__attribute__((noinline)) void sink(unsigned short value) {
  marker += (unsigned char)value;
}

int main(void) {
  const char value[] = {3, 4, 0};
  if (wrapper(value) != 7) {
    return 1;
  }
  if (wrapper_with_work(value) != 7 || marker != 1) {
    return 2;
  }
  if (changed_argument(9) != 10) {
    return 3;
  }
  if (changed_byte(6) != 7) {
    return 4;
  }
  sink(1);
  if (marker != 2) {
    return 5;
  }
  return 0;
}
SRC

function_body() {
  local function_name=$1
  local assembly=$2
  awk "/^${function_name}:$/{inside=1; next} \
       /^\\.func_end_${function_name}:$/{inside=0} \
       inside" "$assembly"
}

for target in 6502 65c02; do
  assembly="$WORK/$target.s"
  "$DAVECC" -target "$target" -S "$SOURCE" -o "$assembly"

  wrapper_body=$(function_body wrapper "$assembly")
  if ! grep -Eq 'jmp[[:space:]]+callee' <<<"$wrapper_body"; then
    echo "$target: forwarding wrapper was not lowered to jmp" >&2
    exit 1
  fi
  if grep -Eq 'jsr|__enter|__leave|__push' <<<"$wrapper_body"; then
    echo "$target: forwarding wrapper retained call/frame overhead" >&2
    exit 1
  fi

  work_body=$(function_body wrapper_with_work "$assembly")
  if ! grep -Eq 'jsr[[:space:]]+__leave' <<<"$work_body" ||
     ! grep -Eq 'jmp[[:space:]]+callee' <<<"$work_body"; then
    echo "$target: framed forwarding tail call was not lowered safely" >&2
    exit 1
  fi

  changed_body=$(function_body changed_argument "$assembly")
  if ! grep -Eq 'jsr[[:space:]]+callee_number' <<<"$changed_body" ||
     grep -Eq 'jmp[[:space:]]+callee_number' <<<"$changed_body"; then
    echo "$target: modified argument was incorrectly tail-forwarded" >&2
    exit 1
  fi
  if grep -Eq 'jsr[[:space:]]+__incsp2' <<<"$changed_body"; then
    echo "$target: fixed scalar callee retained caller cleanup" >&2
    exit 1
  fi
  if ! grep -Eq 'jsr[[:space:]]+__load_result_value2' <<<"$changed_body" ||
     grep -Eq 'jsr[[:space:]]+__load_result([[:space:]]|$)' <<<"$changed_body"; then
    echo "$target: scalar return did not use the fused result helper" >&2
    exit 1
  fi

  changed_byte_body=$(function_body changed_byte "$assembly")
  if ! grep -Eq 'jsr[[:space:]]+__load_result_value1' \
       <<<"$changed_byte_body"; then
    echo "$target: byte return did not use the fused result helper" >&2
    exit 1
  fi

  main_body=$(function_body main "$assembly")
  if ! awk '/jsr[[:space:]]+sink/ {
              getline;
              if ($0 ~ /jsr[[:space:]]+__incsp2/) found=1
            }
            END { exit !found }' <<<"$main_body"; then
    echo "$target: void call did not retain caller cleanup" >&2
    exit 1
  fi

  unoptimized="$WORK/${target}_O0.s"
  "$DAVECC" -target "$target" -O0 -S "$SOURCE" -o "$unoptimized"
  unoptimized_wrapper=$(function_body wrapper "$unoptimized")
  if ! grep -Eq 'jsr[[:space:]]+callee' <<<"$unoptimized_wrapper"; then
    echo "$target: explicit -O0 did not override the O2 target default" >&2
    exit 1
  fi
done

CHAR_TRAITS_SOURCE="$WORK/char_traits.cc"
cat >"$CHAR_TRAITS_SOURCE" <<'SRC'
#include <__char_traits>

using LengthFunction = unsigned short (*)(const char*);
LengthFunction length_function = &std::char_traits<char>::length;

int main() {
  return length_function("abc") == 3 ? 0 : 1;
}
SRC

for target in 6502 65c02; do
  assembly="$WORK/${target}_char_traits.s"
  "$DAVECC" -target "$target" -S -std=c++20 \
    "$CHAR_TRAITS_SOURCE" -o "$assembly"
  length_body=$(
    function_body _ZN3std17char_traits_char_6lengthEPKh "$assembly"
  )
  if ! grep -Eq 'jmp[[:space:]]+strlen' <<<"$length_body" ||
     grep -Eq 'jsr|__enter|__leave|__push' <<<"$length_body"; then
    echo "$target: char_traits::length was not reduced to jmp strlen" >&2
    exit 1
  fi
done

REFERENCE_CHAIN_SOURCE="$WORK/reference_chain.cc"
cat >"$REFERENCE_CHAIN_SOURCE" <<'SRC'
struct Stream {
  int value;
};

[[gnu::noinline]] Stream& write(Stream& output, const char*) {
  return output;
}

Stream global_stream;

int main() {
  return &write(write(global_stream, "first"), "second") == &global_stream
             ? 0
             : 1;
}
SRC

for target in 6502 65c02; do
  assembly="$WORK/${target}_reference_chain.s"
  "$DAVECC" -target "$target" -S -std=c++20 \
    "$REFERENCE_CHAIN_SOURCE" -o "$assembly"
  main_body=$(function_body main "$assembly")
  if [[ $(grep -Ec 'jsr[[:space:]]+__pushi0' <<<"$main_body") -lt 1 ]]; then
    echo "$target: reference-returning call was copied before being pushed" >&2
    exit 1
  fi
done

ADDRESS_PUSH_SOURCE="$WORK/address_push.c"
cat >"$ADDRESS_PUSH_SOURCE" <<'SRC'
__attribute__((noinline)) unsigned short read_value(unsigned short* value) {
  return *value;
}

__attribute__((noinline)) unsigned short read_indirect(unsigned short** value) {
  return **value;
}

__attribute__((noinline)) unsigned short pass_argument(unsigned short* value) {
  return read_indirect(&value);
}

int main(void) {
  unsigned short value = 37;
  return read_value(&value) == 37 ? 0 : 1;
}
SRC

for target in 6502 65c02; do
  assembly="$WORK/${target}_address_push.s"
  "$DAVECC" -target "$target" -S "$ADDRESS_PUSH_SOURCE" -o "$assembly"
  if ! grep -Eq 'jsr[[:space:]]+__var_addr_push_i[0-9]+' "$assembly" ||
     ! grep -Eq 'jsr[[:space:]]+__arg_addr_push' "$assembly"; then
    echo "$target: address materialization and push were not combined" >&2
    exit 1
  fi
done

STACK_REPLACE_SOURCE="$WORK/stack_replace.cc"
cat >"$STACK_REPLACE_SOURCE" <<'SRC'
struct Box {
  int value;
};

[[gnu::noinline]] Box make_box(int value) {
  Box result = {value};
  return result;
}

[[gnu::noinline]] Box make_box2(int first, int second) {
  Box result = {first + second};
  return result;
}

struct Reader {
  [[gnu::noinline]] int read(Box&& box) {
    return box.value;
  }
};

[[gnu::noinline]] int make_and_read() {
  Box box = make_box(41);
  return box.value;
}

int main() {
  Reader reader;
  if (make_and_read() != 41) {
    return 1;
  }
  if (reader.read(make_box(37)) != 37) {
    return 2;
  }
  return reader.read(make_box2(18, 24)) == 42 ? 0 : 3;
}
SRC

for target in 6502 65c02; do
  assembly="$WORK/${target}_stack_replace.s"
  "$DAVECC" -target "$target" -S -std=c++20 \
    "$STACK_REPLACE_SOURCE" -o "$assembly"
  if ! grep -Eq 'jsr[[:space:]]+__pullreg2_drop2' "$assembly" ||
     ! grep -Eq 'jsr[[:space:]]+__replace_top_reg2' "$assembly" ||
     ! grep -Eq 'jsr[[:space:]]+__var_addr_push_i[0-9]+' "$assembly"; then
    echo "$target: struct-result stack operations were not combined" >&2
    exit 1
  fi
  if awk 'previous ~ /jsr[[:space:]]+__pullreg2$/ &&
          /jsr[[:space:]]+__incsp2$/ {found=1}
          NF {previous=$0}
          END {exit found ? 0 : 1}' "$assembly"; then
    echo "$target: pull-result and argument-drop calls were not fused" >&2
    exit 1
  fi
  stack_main=$(function_body main "$assembly")
  if grep -Eq 'jsr[[:space:]]+__var_addr_i[0-9]+' <<<"$stack_main"; then
    echo "$target: dead argument address was materialized before its push" >&2
    exit 1
  fi
  make_box_body=$(function_body _Z8make_boxi "$assembly")
  if ! grep -Eq 'jsr[[:space:]]+__arg_value2_i[0-9]+' \
       <<<"$make_box_body"; then
    echo "$target: struct return address used generic argument load" >&2
    exit 1
  fi
done

MEMORY_HELPER_SOURCE="$WORK/memory_helpers.c"
cat >"$MEMORY_HELPER_SOURCE" <<'SRC'
struct Small {
  unsigned char bytes[5];
};

struct Large {
  unsigned char bytes[300];
};

__attribute__((noinline)) struct Small make_small(void) {
  struct Small value = {{5, 0, 0, 0, 9}};
  return value;
}

__attribute__((noinline)) struct Large make_large(void) {
  struct Large value = {{0}};
  value.bytes[0] = 19;
  value.bytes[255] = 23;
  value.bytes[299] = 29;
  return value;
}

__attribute__((noinline)) unsigned short consume_small(struct Small value) {
  return (unsigned short)value.bytes[0] + value.bytes[4];
}

__attribute__((noinline)) unsigned short consume_large(struct Large value) {
  return (unsigned short)value.bytes[0] + value.bytes[255] + value.bytes[299];
}

int main(void) {
  struct Small small_src = {{3, 0, 0, 0, 7}};
  struct Small small_dest;
  struct Small small_zero = {{0}};
  small_dest = small_src;
  if (consume_small(small_dest) != 10 || small_zero.bytes[4] != 0) {
    return 1;
  }
  if (consume_small(make_small()) != 14) {
    return 2;
  }

  struct Large large_src;
  struct Large large_dest;
  struct Large large_zero = {{0}};
  large_src.bytes[0] = 11;
  large_src.bytes[255] = 13;
  large_src.bytes[299] = 17;
  large_dest = large_src;
  if (consume_large(large_dest) != 41 ||
      large_zero.bytes[0] != 0 ||
      large_zero.bytes[255] != 0 ||
      large_zero.bytes[299] != 0) {
    return 3;
  }
  if (consume_large(make_large()) != 71) {
    return 4;
  }
  return 0;
}
SRC

for target in 6502 65c02; do
  assembly="$WORK/${target}_memory_helpers.s"
  "$DAVECC" -target "$target" -S "$MEMORY_HELPER_SOURCE" -o "$assembly"
  for helper in copymem1 copymem2 pushmem1 pushmem2 zeromem1 zeromem2; do
    if ! grep -A1 -E "jsr[[:space:]]+__${helper}([[:space:]]|$)" \
         "$assembly" | grep -Eq '\.byte[[:space:]]'; then
      echo "$target: __$helper did not use an inline descriptor" >&2
      exit 1
    fi
  done
  if grep -B12 -E 'jsr[[:space:]]+__(copy|push|zero)mem[12]' "$assembly" |
     grep -Eq 'sta[[:space:]]+__mem_(src|dest|size)'; then
    echo "$target: memory helper retained zero-page argument setup" >&2
    exit 1
  fi
done

"$DAVECC" -target 65c02 "$SOURCE" "$LIBC" -o "$WORK/tail_call.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/tail_call.exe"

"$DAVECC" -target 65c02 -std=c++20 \
  "$REFERENCE_CHAIN_SOURCE" "$LIBC" -o "$WORK/reference_chain.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/reference_chain.exe"

"$DAVECC" -target 65c02 \
  "$ADDRESS_PUSH_SOURCE" "$LIBC" -o "$WORK/address_push.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/address_push.exe"

"$DAVECC" -target 65c02 -std=c++20 \
  "$STACK_REPLACE_SOURCE" "$LIBC" -o "$WORK/stack_replace.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/stack_replace.exe"

"$DAVECC" -target 65c02 \
  "$MEMORY_HELPER_SOURCE" "$LIBC" -o "$WORK/memory_helpers.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/memory_helpers.exe"
