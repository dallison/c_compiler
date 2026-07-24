#!/usr/bin/env bash

set -euo pipefail

davecc=$1
elfdump=$2

# Resolve libc headers from Bazel runfiles.
if [[ -n "${RUNFILES_DIR:-}" ]]; then
  include_dir="$RUNFILES_DIR/_main/libc/include"
elif [[ -n "${TEST_SRCDIR:-}" ]]; then
  include_dir="$TEST_SRCDIR/_main/libc/include"
else
  include_dir="$(cd "$(dirname "$davecc")/.." && pwd)/libc/include"
fi
if [[ ! -d "$include_dir" ]]; then
  echo "unable to find libc headers at $include_dir" >&2
  exit 1
fi

compile_cpp() {
  local target=$1
  shift
  "$davecc" -target "$target" -std=c++20 -O1 -isystem "$include_dir" "$@"
}

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cat >"$work/abi_gates.cpp" <<'EOF'
struct Pod {
  int value;
};

int catch_all(void) {
  try {
    throw 1;
  } catch (...) {
    return 0;
  }
}

int catch_typed(void) {
  try {
    throw Pod{42};
  } catch (const Pod& caught) {
    return caught.value == 42 ? 0 : 1;
  }
}

int main(void) {
  return catch_all() + catch_typed();
}
EOF

require_section() {
  local file=$1
  local section=$2
  local label=$3
  if ! grep -F "$section" "$file" >/dev/null; then
    echo "$label is missing section $section" >&2
    sed -n '1,120p' "$file" >&2
    return 1
  fi
}

require_symbol() {
  local file=$1
  local symbol=$2
  local label=$3
  if ! grep -F "$symbol" "$file" >/dev/null; then
    echo "$label is missing symbol $symbol" >&2
    sed -n '1,160p' "$file" >&2
    return 1
  fi
}

require_absent_section() {
  local file=$1
  local section=$2
  local label=$3
  if grep -F "$section" "$file" >/dev/null; then
    echo "$label unexpectedly contains section $section" >&2
    sed -n '1,120p' "$file" >&2
    return 1
  fi
}

require_absent_symbol() {
  local file=$1
  local symbol=$2
  local label=$3
  if grep -F "$symbol" "$file" >/dev/null; then
    echo "$label unexpectedly contains symbol $symbol" >&2
    sed -n '1,160p' "$file" >&2
    return 1
  fi
}

check_elf64_target() {
  local target=$1
  local obj="$work/$target.o"
  local exe="$work/$target.exe"
  local dso="$work/$target.so"

  compile_cpp "$target" -c "$work/abi_gates.cpp" -o "$obj"
  compile_cpp "$target" -static -Wl,-e -Wl,main \
    "$work/abi_gates.cpp" -o "$exe"
  compile_cpp "$target" -shared "$work/abi_gates.cpp" -o "$dso"

  "$elfdump" -S "$obj" >"$work/$target.obj.sections"
  "$elfdump" -s "$obj" >"$work/$target.obj.symbols"
  "$elfdump" -S "$exe" >"$work/$target.exe.sections"
  "$elfdump" -s "$exe" >"$work/$target.exe.symbols"
  "$elfdump" -S "$dso" >"$work/$target.dso.sections"
  "$elfdump" -s "$dso" >"$work/$target.dso.symbols"

  require_section "$work/$target.obj.sections" ".eh_frame" "$target object"
  require_section "$work/$target.obj.sections" ".gcc_except_table" "$target object"
  require_absent_section "$work/$target.obj.sections" ".davecc_except_table" \
    "$target object"
  require_symbol "$work/$target.obj.symbols" "_ZTS" "$target object RTTI gate"
  require_symbol "$work/$target.obj.symbols" "_ZTI" "$target object RTTI gate"
  require_absent_symbol "$work/$target.obj.symbols" "__davecc_typeinfo_" \
    "$target object"
  require_absent_symbol "$work/$target.obj.symbols" \
    "__davecc_current_exception_" "$target object"
  require_absent_symbol "$work/$target.obj.symbols" "__davecc_throw" \
    "$target object"

  require_section "$work/$target.exe.sections" ".eh_frame" "$target executable"
  require_section "$work/$target.exe.sections" ".gcc_except_table" \
    "$target executable"
  require_absent_section "$work/$target.exe.sections" ".davecc_except_table" \
    "$target executable"
  require_symbol "$work/$target.exe.symbols" "__eh_frame_start" \
    "$target executable"
  require_symbol "$work/$target.exe.symbols" "__eh_frame_end" \
    "$target executable"
  require_symbol "$work/$target.exe.symbols" "__gcc_except_table_start" \
    "$target executable"
  require_symbol "$work/$target.exe.symbols" "__gcc_except_table_end" \
    "$target executable"
  require_absent_symbol "$work/$target.exe.symbols" \
    "__davecc_except_table_start" "$target executable"
  require_absent_symbol "$work/$target.exe.symbols" \
    "__davecc_except_table_end" "$target executable"
  require_symbol "$work/$target.exe.symbols" "__gxx_personality_v0" \
    "$target executable"
  require_absent_symbol "$work/$target.exe.symbols" "__davecc_typeinfo_" \
    "$target executable"
  require_absent_symbol "$work/$target.exe.symbols" \
    "__davecc_current_exception_" "$target executable"
  require_absent_symbol "$work/$target.exe.symbols" "__davecc_throw" \
    "$target executable"
  require_section "$work/$target.dso.sections" ".eh_frame" "$target DSO"
  require_section "$work/$target.dso.sections" ".gcc_except_table" "$target DSO"
  require_symbol "$work/$target.dso.symbols" "__eh_frame_start" "$target DSO"
  require_symbol "$work/$target.dso.symbols" "__eh_frame_end" "$target DSO"
}

check_arm_target() {
  local obj="$work/arm.o"
  local dso="$work/arm.so"

  compile_cpp arm -c "$work/abi_gates.cpp" -o "$obj"
  compile_cpp arm -shared "$work/abi_gates.cpp" -o "$dso"

  "$elfdump" -S "$obj" >"$work/arm.obj.sections"
  "$elfdump" -s "$obj" >"$work/arm.obj.symbols"
  "$elfdump" -S "$dso" >"$work/arm.dso.sections"
  "$elfdump" -s "$dso" >"$work/arm.dso.symbols"

  require_section "$work/arm.obj.sections" ".ARM.exidx" "arm object"
  require_section "$work/arm.obj.sections" ".ARM.extab" "arm object"
  require_absent_section "$work/arm.obj.sections" ".davecc_except_table" \
    "arm object"
  require_absent_section "$work/arm.obj.sections" ".eh_frame" \
    "arm object (EHABI target)"
  require_absent_section "$work/arm.obj.sections" ".gcc_except_table" \
    "arm object (EHABI target)"
  require_absent_symbol "$work/arm.obj.symbols" "__davecc_typeinfo_" \
    "arm object"
  require_section "$work/arm.dso.sections" ".ARM.exidx" "arm DSO"
  require_section "$work/arm.dso.sections" ".ARM.extab" "arm DSO"
  require_symbol "$work/arm.dso.symbols" "__exidx_start" "arm DSO"
  require_symbol "$work/arm.dso.symbols" "__exidx_end" "arm DSO"
}

check_elf64_target x86_64
check_elf64_target aarch64
check_elf64_target riscv
check_arm_target

echo "exception ABI migration gates passed"
