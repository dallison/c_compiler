#!/bin/bash
# End-to-end weak binding coverage for assembler, linker, and C++ inline ODR.
set -euo pipefail

DAVECC=""
ELFDUMP=""
LIBC=""
INTERPRETER=""
SUITE_ROOT=""
TIMEOUT=30
declare -a INTERP_ARGS=()

usage() {
  echo "usage: $0 --davecc PATH --elfdump PATH --libc PATH --interpreter PATH [--suite-root PATH]" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --davecc) DAVECC=$2; shift 2 ;;
    --elfdump) ELFDUMP=$2; shift 2 ;;
    --libc) LIBC=$2; shift 2 ;;
    --interpreter) INTERPRETER=$2; shift 2 ;;
    --suite-root) SUITE_ROOT=$2; shift 2 ;;
    --timeout) TIMEOUT=$2; shift 2 ;;
    --interp-arg) INTERP_ARGS+=("$2"); shift 2 ;;
    -h|--help) usage ;;
    *) echo "unknown option: $1" >&2; usage ;;
  esac
done

if [ -z "$DAVECC" ] || [ -z "$ELFDUMP" ] || [ -z "$LIBC" ] ||
   [ -z "$INTERPRETER" ]; then
  usage
fi

resolve_runfile() {
  local path=$1
  if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
    local rooted="${TEST_SRCDIR}/${TEST_WORKSPACE}/${path}"
    if [ -e "$rooted" ]; then
      echo "$rooted"
      return
    fi
  fi
  echo "$path"
}

DAVECC=$(resolve_runfile "$DAVECC")
ELFDUMP=$(resolve_runfile "$ELFDUMP")
LIBC=$(resolve_runfile "$LIBC")
INTERPRETER=$(resolve_runfile "$INTERPRETER")
if [ -n "$SUITE_ROOT" ]; then
  SUITE_ROOT=$(resolve_runfile "$SUITE_ROOT")
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  SUITE_ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE}/cxx_testsuite"
else
  SUITE_ROOT="cxx_testsuite"
fi

INCLUDE_DIR="$SUITE_ROOT/../libc/include"
if [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  INCLUDE_DIR="${TEST_SRCDIR}/${TEST_WORKSPACE}/libc/include"
fi

declare -a TIMEOUT_CMD=()
if command -v gtimeout >/dev/null 2>&1; then
  TIMEOUT_CMD=(gtimeout "$TIMEOUT")
elif command -v timeout >/dev/null 2>&1; then
  TIMEOUT_CMD=(timeout "$TIMEOUT")
fi

work=$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-weak.XXXXXX")
trap 'rm -rf "$work"' EXIT

compile_obj() {
  "$DAVECC" -target x86_64 -c "$1" -o "$2"
}

compile_cxx_obj() {
  "$DAVECC" -target x86_64 -std=c++20 -isystem "$INCLUDE_DIR" -c "$1" -o "$2"
}

link_and_run() {
  local bin=$1
  shift
  "$DAVECC" -target x86_64 -static "$@" "$LIBC" -o "$bin" -Wl,-e -Wl,main
  "${TIMEOUT_CMD[@]}" "$INTERPRETER" "${INTERP_ARGS[@]}" "$bin"
}

echo "=== weak binding tests ==="

cat >"$work/weak_asm.s" <<'EOF'
	.text
	.weak weak_asm_func
	.type weak_asm_func, @function
weak_asm_func:
	ret
EOF
compile_obj "$work/weak_asm.s" "$work/weak_asm.o"
"$ELFDUMP" -s "$work/weak_asm.o" >"$work/weak_asm.symbols"
if ! grep -q "weal.*weak_asm_func" "$work/weak_asm.symbols"; then
  echo "FAIL assembler weak symbol binding"
  sed 's/^/  /' "$work/weak_asm.symbols"
  exit 1
fi
echo "ok assembler emits STB_WEAK"

cat >"$work/weak_choose.c" <<'EOF'
__attribute__((weak)) int choose(void) {
  return 1;
}
EOF
cat >"$work/strong_choose.c" <<'EOF'
int choose(void) {
  return 2;
}
EOF
cat >"$work/choose_main.c" <<'EOF'
int choose(void);
int main(void) {
  return choose() == 2 ? 0 : choose();
}
EOF
compile_obj "$work/weak_choose.c" "$work/weak_choose.o"
compile_obj "$work/strong_choose.c" "$work/strong_choose.o"
compile_obj "$work/choose_main.c" "$work/choose_main.o"
link_and_run "$work/choose.bin" \
  "$work/choose_main.o" "$work/weak_choose.o" "$work/strong_choose.o"
echo "ok strong definition overrides weak definition"

cat >"$work/weak_same_a.c" <<'EOF'
__attribute__((weak)) int weak_same(void) {
  return 3;
}
EOF
cat >"$work/weak_same_b.c" <<'EOF'
__attribute__((weak)) int weak_same(void) {
  return 3;
}
EOF
cat >"$work/weak_same_main.c" <<'EOF'
int weak_same(void);
int main(void) {
  return weak_same() == 3 ? 0 : weak_same();
}
EOF
compile_obj "$work/weak_same_a.c" "$work/weak_same_a.o"
compile_obj "$work/weak_same_b.c" "$work/weak_same_b.o"
compile_obj "$work/weak_same_main.c" "$work/weak_same_main.o"
link_and_run "$work/weak_same.bin" \
  "$work/weak_same_main.o" "$work/weak_same_a.o" "$work/weak_same_b.o"
echo "ok duplicate weak definitions link"

cat >"$work/weak_missing.c" <<'EOF'
extern int missing_weak(void) __attribute__((weak));
int main(void) {
  return missing_weak == 0 ? 0 : 1;
}
EOF
compile_obj "$work/weak_missing.c" "$work/weak_missing.o"
link_and_run "$work/weak_missing.bin" "$work/weak_missing.o"
echo "ok unresolved weak references resolve to zero"

cat >"$work/inline_shared.h" <<'EOF'
extern int local_static_ctor_count;

inline int shared_inline_value(void) {
  return 4;
}

inline int override_inline_value(void) {
  return 5;
}

inline int shared_inline_counter(void) {
  static int counter = 0;
  counter = counter + 1;
  return counter;
}

struct InlineLocalBox {
  int value;
  InlineLocalBox() : value(10) {
    local_static_ctor_count = local_static_ctor_count + 1;
  }
};

inline int shared_inline_box_value(void) {
  static InlineLocalBox box;
  int result = box.value;
  box.value = box.value + 1;
  return result;
}

inline int shared_inline_variable = 7;
inline int zero_initialized_inline_variable;

namespace inline_variable_ns {
inline int namespaced_inline_variable = 11;
}

struct InlineStaticData {
  inline static int value = 13;
  static constexpr int constexpr_value = 17;
};

struct OutOfClassStaticData {
  static int value;
};
EOF
cat >"$work/inline_a.cpp" <<'EOF'
#include "inline_shared.h"
int value = 100;
int OutOfClassStaticData::value = 19;
int inline_a(void) {
  return shared_inline_value();
}
int inline_override_user(void) {
  return override_inline_value();
}
int inline_counter_a(void) {
  return shared_inline_counter();
}
int inline_box_a(void) {
  return shared_inline_box_value();
}
int inline_variables_a(void) {
  shared_inline_variable = shared_inline_variable + 1;
  zero_initialized_inline_variable = zero_initialized_inline_variable + 2;
  InlineStaticData::value = InlineStaticData::value + 3;
  inline_variable_ns::namespaced_inline_variable =
      inline_variable_ns::namespaced_inline_variable + 4;
  return shared_inline_variable + zero_initialized_inline_variable +
         InlineStaticData::value +
         inline_variable_ns::namespaced_inline_variable +
         InlineStaticData::constexpr_value + OutOfClassStaticData::value +
         value;
}
EOF
cat >"$work/inline_b.cpp" <<'EOF'
#include "inline_shared.h"
int inline_b(void) {
  return shared_inline_value();
}
int inline_counter_b(void) {
  return shared_inline_counter();
}
int inline_box_b(void) {
  return shared_inline_box_value();
}
int inline_variables_b(void) {
  shared_inline_variable = shared_inline_variable + 5;
  zero_initialized_inline_variable = zero_initialized_inline_variable + 6;
  InlineStaticData::value = InlineStaticData::value + 7;
  inline_variable_ns::namespaced_inline_variable =
      inline_variable_ns::namespaced_inline_variable + 8;
  return shared_inline_variable + zero_initialized_inline_variable +
         InlineStaticData::value +
         inline_variable_ns::namespaced_inline_variable +
         InlineStaticData::constexpr_value;
}
EOF
cat >"$work/inline_strong.cpp" <<'EOF'
int override_inline_value(void) {
  return 9;
}
EOF
cat >"$work/inline_main.cpp" <<'EOF'
int inline_a(void);
int inline_b(void);
int inline_override_user(void);
int inline_counter_a(void);
int inline_counter_b(void);
int inline_box_a(void);
int inline_box_b(void);
int inline_variables_a(void);
int inline_variables_b(void);
int local_static_ctor_count = 0;
int main(void) {
  int result = inline_a() + inline_b() + inline_override_user() +
               inline_counter_a() + inline_counter_b() +
               inline_box_a() + inline_box_b() +
               inline_variables_a() + inline_variables_b() +
               local_static_ctor_count;
  return result == 303 ? 0 : result;
}
EOF
compile_cxx_obj "$work/inline_a.cpp" "$work/inline_a.o"
compile_cxx_obj "$work/inline_b.cpp" "$work/inline_b.o"
compile_cxx_obj "$work/inline_strong.cpp" "$work/inline_strong.o"
compile_cxx_obj "$work/inline_main.cpp" "$work/inline_main.o"
link_and_run "$work/inline.bin" \
  "$work/inline_main.o" "$work/inline_a.o" "$work/inline_b.o" \
  "$work/inline_strong.o"
echo "ok C++ inline definitions link as weak definitions"

echo "=== weak binding tests passed ==="
