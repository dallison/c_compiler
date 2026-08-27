#!/usr/bin/env bash
# Ways a dynamic program reaches something in a shared object, all of which were
# broken while a direct call through the PLT worked, so nothing noticed.
#
#  - A data word initialized to the address of an imported symbol.  The linker
#    emitted a relative relocation, which carries the whole link-time value and
#    has no room for a symbol, for an address that is not known until the loader
#    resolves it.  The addend came out 0 and the loader wrote the load base into
#    the slot: 0 for a fixed-address executable, so calling through a function
#    pointer silently did nothing instead of faulting.
#
#  - A shared object with a constructor.  The loader rebases the dynamic section
#    tags it reads as addresses, and DT_INIT_ARRAY was not in the ELF64 list, so
#    the init array was read from its link-time address and the program died in
#    the loader before reaching main.  ELF32, and so ARM, translated it already.
#
#  - Code naming an imported object directly rather than through a pointer.  On
#    aarch64 that computed the address from the PC, which reaches only what this
#    image reserved for the name, so an imported object read as 0.  The address
#    has to come from the GOT, the only place the loader can write it.
set -euo pipefail

if [[ $# -ne 6 ]]; then
  echo "usage: $0 davecc aarch64 arm riscv x86_64 pcode" >&2
  exit 2
fi

DAVECC="$1"
AARCH64="$2"
ARM="$3"
RISCV="$4"
X86_64="$5"
PCODE="$6"

WORK="${TEST_TMPDIR:-/tmp}/dynamic_dso_data"
mkdir -p "$WORK"

# One shared object rather than two, because linking against two faults on ARM,
# which is an open defect of its own.  It carries a constructor as well as the
# data and functions, so both its init array and the executable's have to run.
cat > "$WORK/lib.c" <<'SRC'
int imported_value = 40;
int imported_array[4] = {10, 20, 30, 40};
int library_constructed = 0;
int Imported(int value) { return value + 1; }
int ImportedTwo(int value) { return value + 2; }
__attribute__((constructor))
static void LibraryConstruct(void) { library_constructed = 60; }
SRC

# Every kind of data word that holds an address, interleaved, since the relative
# ones are emitted before the ones naming a symbol and a wrong count or order in
# DT_RELACOUNT shows up as one of them being skipped.
cat > "$WORK/main.c" <<'SRC'
extern int imported_value;
extern int imported_array[4];
extern int library_constructed;
int Imported(int value);
int ImportedTwo(int value);

int local_value = 7;
int constructed = 0;

int* local_pointer = &local_value;
int* imported_pointer = &imported_value;
int* another_local_pointer = &local_value;
int* imported_element_pointer = &imported_array[3];
int* library_constructed_pointer = &library_constructed;
int (*function_pointer)(int) = Imported;
int (*function_table[2])(int) = {Imported, ImportedTwo};

__attribute__((constructor))
static void Construct(void) { constructed = 39; }

int main(void) {
  if (*local_pointer != 7 || *another_local_pointer != 7) return 1;
  if (*imported_pointer != 40) return 2;
  if (*imported_element_pointer != 40) return 3;
  if (function_pointer(20) != 21) return 4;
  if (function_table[1](30) != 32) return 5;
  if (Imported(10) != 11) return 6;
  // Named directly, so the address comes from the code rather than from a word
  // the loader relocated.  A local definition is here too because it takes the
  // other path through the same code and must keep working.
  if (imported_value != 40) return 7;
  if (imported_array[3] != 40) return 8;
  if (local_value != 7) return 9;
  imported_value = 41;
  if (imported_value != 41) return 10;
  // 39 from this image's constructor and 60 from the shared object's.
  return constructed + *library_constructed_pointer;
}
SRC

run_one() {
  local target="$1"
  local interpreter="$2"
  local library="$WORK/lib-$target.so"
  local executable="$WORK/dso-data-$target.exe"

  "$DAVECC" -target "$target" -nostdinc -fpic -shared \
    "$WORK/lib.c" -o "$library"
  "$DAVECC" -target "$target" -nostdinc -nostdlib -fpic \
    -Wl,-e -Wl,main -rpath "$WORK" \
    "$WORK/main.c" "$library" -o "$executable"

  # Eager binding writes the same slots as the lazy resolver from different
  # code, so run both ways as //:dynamic_interpreters_test does.  That test also
  # runs the interpreted engine with -i; this one does not, because on aarch64
  # the lazy resolver loses the first argument register there, which is an open
  # defect of its own and has nothing to do with what this test covers.
  local bind_now
  for bind_now in "" 1; do
    local rc=0
    LD_BIND_NOW="$bind_now" "$interpreter" "$executable" || rc=$?
    if [[ "$rc" -ne 99 ]]; then
      echo "$target returned $rc; expected 99 (LD_BIND_NOW='$bind_now')." \
        "1-3 is a data word holding an address, 4-6 a call," \
        "7-10 an object named directly," \
        "60 means this image's constructor did not run and 39 the library's" >&2
      exit 1
    fi
  done
}

run_one aarch64 "$AARCH64"
run_one arm "$ARM"
run_one riscv "$RISCV"
run_one x86_64 "$X86_64"
run_one pcode "$PCODE"
