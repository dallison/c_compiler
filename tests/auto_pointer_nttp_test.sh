#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/auto-pointer-nttp"
mkdir -p "$WORK"

cat >"$WORK/positive.cpp" <<'SRC'
int object;
int function(int value) { return value; }
struct Record {
  int field;
  int method(int value) { return field + value; }
};

template <auto Value> int identity() { return 1; }
template <const int* Value> struct ConstPointer {};
template <typename T, auto FieldMember = nullptr,
          auto NamedMember = FieldMember, typename Factory = void>
struct Metadata {};

int use() {
  ConstPointer<&object> qualified_pointer;
  Metadata<Record> empty;
  Metadata<Record, &Record::field> forwarded;
  Metadata<Record, (int Record::*)nullptr> null_member;
  return identity<1>() + identity<1L>() + identity<nullptr>() +
         identity<(int*)nullptr>() +
         identity<&object>() + identity<&function>() +
         identity<&Record::field>() + identity<&Record::method>() +
         sizeof(qualified_pointer) + sizeof(empty) + sizeof(forwarded) +
         sizeof(null_member);
}
SRC

"$DAVECC" -target x86_64 -std=c++17 -nostdinc -c "$WORK/positive.cpp" \
  -o "$WORK/positive.o"
"$ELFDUMP" -s "$WORK/positive.o" >"$WORK/symbols.txt"
grep -Fq 'identityILi1EE' "$WORK/symbols.txt"
grep -Fq 'identityILl1EE' "$WORK/symbols.txt"
grep -Fq 'identityILDnEE' "$WORK/symbols.txt"
grep -Fq 'identityILPi0EE' "$WORK/symbols.txt"
grep -Fq 'identityIL_Z6objectEE' "$WORK/symbols.txt"
grep -Fq 'identityIL_Z8functioniEE' "$WORK/symbols.txt"
grep -Fq 'identityIXadL_ZN6Record5fieldEEEE' "$WORK/symbols.txt"
grep -Fq 'identityIXadL_ZN6Record6methodEiEEE' "$WORK/symbols.txt"

cat >"$WORK/local.cpp" <<'SRC'
template <auto Value> struct Holder {};
int f() {
  int local = 0;
  Holder<&local> invalid;
  return sizeof(invalid);
}
SRC
if "$DAVECC" -target x86_64 -std=c++17 -nostdinc -c "$WORK/local.cpp" \
     -o "$WORK/local.o" >"$WORK/local.log" 2>&1; then
  echo "local object address was accepted as an NTTP" >&2
  exit 1
fi
grep -Fq 'must be a constant expression' "$WORK/local.log"

cat >"$WORK/mismatch.cpp" <<'SRC'
int function();
template <int* Value> struct Holder {};
Holder<&function> invalid;
SRC
if "$DAVECC" -target x86_64 -std=c++17 -nostdinc -c "$WORK/mismatch.cpp" \
     -o "$WORK/mismatch.o" >"$WORK/mismatch.log" 2>&1; then
  echo "type-incompatible pointer NTTP was accepted" >&2
  exit 1
fi
grep -Fq 'not compatible with parameter type' "$WORK/mismatch.log"

cat >"$WORK/floating.cpp" <<'SRC'
template <auto Value> struct Holder {};
Holder<1.5> invalid;
SRC
if "$DAVECC" -target x86_64 -std=c++17 -nostdinc -c "$WORK/floating.cpp" \
     -o "$WORK/floating.o" >"$WORK/floating.log" 2>&1; then
  echo "floating-point NTTP was accepted" >&2
  exit 1
fi
grep -Fq 'must be a constant expression' "$WORK/floating.log"

cat >"$WORK/pre17.cpp" <<'SRC'
template <auto Value> struct Holder {};
Holder<1> invalid;
SRC
if "$DAVECC" -target x86_64 -std=c++14 -nostdinc -c "$WORK/pre17.cpp" \
     -o "$WORK/pre17.o" >"$WORK/pre17.log" 2>&1; then
  echo "auto NTTP was accepted before C++17" >&2
  exit 1
fi
grep -Fq 'auto non-type template parameters require C++17' "$WORK/pre17.log"

cat >"$WORK/nttp_values.cppm" <<'SRC'
export module nttp_values;
export int module_object;
export struct ModuleRecord { int field; };
export template <auto Value> struct ModuleHolder {};
export using ModuleObjectHolder = ModuleHolder<&module_object>;
export using ModuleMemberHolder = ModuleHolder<&ModuleRecord::field>;
SRC
"$DAVECC" -target x86_64 -std=c++20 -nostdinc \
  -Xemit-module "$WORK/nttp_values.dcm" "$WORK/nttp_values.cppm"

cat >"$WORK/importer.cpp" <<'SRC'
import nttp_values;
ModuleObjectHolder object_holder;
ModuleMemberHolder member_holder;
int use_imports() {
  return sizeof(object_holder) + sizeof(member_holder);
}
SRC
"$DAVECC" -target x86_64 -std=c++20 -nostdinc -c \
  -fprebuilt-module-path "$WORK" "$WORK/importer.cpp" \
  -o "$WORK/importer.o"
