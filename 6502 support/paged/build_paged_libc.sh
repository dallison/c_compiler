#!/bin/bash
# Build the BBC sideways libc image and the main-RAM shim archive.
#
#   build_paged_libc.sh <davecc> <archivist> <elfdump> <outdir> [--update-exports]
#
# The image is linked at 0x8000 as a language ROM. .text, .data, and .bss
# share that 16 KiB window because the bank is sideways RAM. The driver
# target 6502-acorn-bbc-davecc links <outdir>/bbc_start.o,
# <outdir>/libc6502_bbc_runtime.a, <outdir>/libc6502_paged_shim.a, and
# <outdir>/bbc.ld. Each shim is its own section, so --gc-sections drops
# the ones the program does not call.
#
# LIBC_ROM_SLOT (default 4) is the sideways socket the shim selects.
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <davecc> <archivist> <elfdump> <outdir> [--update-exports]" >&2
  exit 2
fi

davecc="$1"
archivist="$2"
elfdump="$3"
out="$4"
update=""
if [[ "${5:-}" == "--update-exports" ]]; then
  update="--update-exports"
fi

root="$(cd "$(dirname "$0")/../.." && pwd)"
sup="$root/6502 support"
paged="$sup/paged"
slot="${LIBC_ROM_SLOT:-4}"

rm -rf "$out"
mkdir -p "$out/runtime" "$out/libc" "$out/extra" "$out/gen"

cflags=(-target 6502 -c -DDAVECC_BBC -DDAVECC_PAGED_LIBC
        -isystem "$root/libc/include" -I"$root/libc")
slot2="${LIBC_ROM2_SLOT:-5}"
slot3="${LIBC_ROM3_SLOT:-6}"
slot4="${LIBC_ROM4_SLOT:-7}"
slot5="${LIBC_ROM5_SLOT:-8}"
slot6="${LIBC_ROM6_SLOT:-9}"
slot7="${LIBC_ROM7_SLOT:-10}"
# libc, libm, two halves of math_extra, the C++ bodies, printf/scanf,
# and the float printers those call.
asmflags=(-target 6502 -c -I"$sup" -I"$paged" -I"$out/gen" \
          -DLIBC_ROM_SLOT="$slot" -DLIBC_ROM2_SLOT="$slot2" \
          -DLIBC_ROM3_SLOT="$slot3" -DLIBC_ROM4_SLOT="$slot4" \
          -DLIBC_ROM5_SLOT="$slot5" -DLIBC_ROM6_SLOT="$slot6" \
          -DLIBC_ROM7_SLOT="$slot7" -DPAGED_ROM_COUNT=7)

runtime_srcs=(
  enter.s var_addr.s var_value.s load_indirect.s load_indirect8.s
  store_indirect.s store_indirect8.s push.s pull_drop.s frame_push.s
  inline_mem_params.s pushmem.s copymem.s zeromem.s incdec.s
  jump_table.s jmpi.s mul.s longmul.s longlongmul.s div.s longdiv.s
  longlongdiv.s spill.s fcommon.s fadd.s fconv.s fcmp.s fneg.s fmul.s
  fdiv.s intrinsic.s setjmp.s stacktrace.s bbc_syscall.s
)

for src in "${runtime_srcs[@]}"; do
  "$davecc" "${asmflags[@]}" "$sup/$src" -o "$out/runtime/$(basename "${src%.s}").o"
done

# Not part of the sideways image. The driver links this as the startup
# object; its weak __libc_paged_ensure_init is replaced by the shim.
"$davecc" "${asmflags[@]}" "$sup/bbc_start.s" -o "$out/runtime/bbc_start.o"

# MOS wrappers stay in main RAM. They are archive members, so a program
# that never calls them does not keep them.
"$davecc" "${asmflags[@]}" "$sup/bbc_mos.s" -o "$out/runtime/bbc_mos.o"

# The sideways bank is 16 KiB. The full libc is far larger (the previous
# link of stdio plus string plus malloc already ran past 0xC000). This is
# the subset that fits: the string and memory routines, malloc, the stdio
# objects, and the literal/simple printf specializations. Full printf,
# scanf, wchar, ctype, and math stay out of the bank.
libc_srcs=(
  calloc.c free.c malloc.c realloc.c
  fflush.c fputc.c fputs.c fwrite.c
  memchr.c memcmp.c memcpy.c memmove.c memset.c
  posix.c posix_close.c posix_open.c posix_read.c
  printf_literal.c printf_simple.c itoa_int.c
  stdio.c stdio_flags.c
  strcat.c strchr.c strcmp.c strcpy.c strlen.c
  strncat.c strncmp.c strncpy.c strrchr.c strspn.c strcspn.c strstr.c strtok.c
  abs.c atoi.c
)

for src in "${libc_srcs[@]}"; do
  "$davecc" "${cflags[@]}" "$root/libc/$src" -o "$out/libc/$(basename "${src%.c}").o"
done

# What does not fit beside the first image. libm, the rest of the C
# library that is not already exported, and the C++ bodies that live in
# their own translation units rather than in headers.
extra_c=(
  acos.c asin.c atan.c atan2.c ceil.c exp.c fabs.c fenv.c floor.c
  frexp.c log.c math_extra.c pow.c sincos.c sqrt.c tan.c ldexp.c modf.c
  complex_core.c complex_exp.c complex_hyperbolic.c complex_inverse.c
  complex_sqrt.c complex_trig.c
  ctype.c
  cxx_new.c cxx_guard.c cxx_rtti.c
  exit.c davecc_lifecycle.c
)
extra_cc=(
  iostream.cc iostream_cin.cc iostream_cout.cc iostream_cerr.cc
  iostream_clog.cc iostream_input.cc
  cxx_new_overloads.cc exception.cc stdexcept.cc
  locale.cc bad_cast.cc
)
for src in "${extra_c[@]}"; do
  "$davecc" "${cflags[@]}" "$root/libc/$src" -o "$out/extra/$(basename "${src%.c}").o"
done
for src in "${extra_cc[@]}"; do
  "$davecc" "${cflags[@]}" -std=c++20 "$root/libc/$src" \
    -o "$out/extra/$(basename "${src%.cc}").o"
done

symfile="$out/gen/symbols.txt"
: >"$symfile"
for obj in "$out/libc"/*.o; do
  "$elfdump" -s "$obj" >>"$symfile"
done

python3 "$paged/gen_shim.py" \
  --symbols "$symfile" \
  --exports "$paged/exports.list" \
  --vectors "$out/gen/vectors.s" \
  --shim "$out/gen/shim_funcs.s" \
  $update

"$davecc" "${asmflags[@]}" "$paged/rom_header.s" -o "$out/gen/rom_header.o"
"$davecc" "${asmflags[@]}" "$paged/rom_init.s" -o "$out/gen/rom_init.o"
"$davecc" "${asmflags[@]}" "$out/gen/vectors.s" -o "$out/gen/vectors.o"
"$davecc" "${asmflags[@]}" -I"$paged" "$out/gen/shim_funcs.s" -o "$out/gen/shim_funcs.o"
"$davecc" "${asmflags[@]}" "$paged/shim_runtime.s" -o "$out/gen/shim_runtime.o"

rom_objs=()
for obj in "$out/runtime"/*.o; do
  base="$(basename "$obj")"
  if [[ "$base" == "bbc_start.o" || "$base" == "bbc_mos.o" ]]; then
    continue
  fi
  rom_objs+=("$obj")
done

"$davecc" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$paged/libc_rom.ld" \
  -e paged_rom_entry \
  "$out/gen/rom_header.o" "$out/gen/rom_init.o" "$out/gen/vectors.o" \
  "${rom_objs[@]}" "$out/libc"/*.o \
  -o "$out/libc6502_paged.elf"

# Separate archive members, still in their own ELF sections. -r would merge
# those sections and --gc-sections could no longer drop an unused stub.
shim_members=("$out/gen/shim_funcs.o" "$out/gen/shim_runtime.o")

"$davecc" "${asmflags[@]}" -DPAGED_SKIP_STREAMS "$paged/rom_init.s" \
  -o "$out/gen/rom_init_extra.o"

# name, rom index, title, cross-export list, cross rom, subset flag, objects...
link_extra() {
  local name="$1" index="$2" title="$3" cross="$4" cross_rom="$5" subset="$6"
  shift 6
  local sym="$out/gen/$name.sym"
  local exports="$paged/exports_$name.list"
  : >"$sym"
  for obj in "$@"; do
    "$elfdump" -s "$obj" >>"$sym"
  done
  local subset_arg=()
  if [[ "$subset" == subset ]]; then
    subset_arg=(--export-subset)
  fi
  local cross_args=()
  if [[ "$cross" != none ]]; then
    cross_args=(--cross-out "$out/gen/cross_$name.s")
    if [[ "$cross" == *","* ]]; then
      local part
      local old_ifs="$IFS"
      IFS=','
      for part in $cross; do
        cross_args+=(--cross "$part")
      done
      IFS="$old_ifs"
    else
      cross_args+=(--cross-exports "$cross" --cross-rom "$cross_rom")
    fi
  fi
  python3 "$paged/gen_shim.py" \
    --symbols "$sym" \
    --exports "$exports" \
    --vectors "$out/gen/vectors_$name.s" \
    --shim "$out/gen/shim_$name.s" \
    --rom-index "$index" \
    ${subset_arg[@]+"${subset_arg[@]}"} \
    ${cross_args[@]+"${cross_args[@]}"} \
    $update
  printf '#define PAGED_NAME "%s"\n#define PAGED_BANNER "%s 1.00"\n' \
    "$title" "$title" >"$out/gen/image_title.h"
  "$davecc" "${asmflags[@]}" -DPAGED_EXTRA "$paged/rom_header.s" \
    -o "$out/gen/rom_header_$name.o"
  "$davecc" "${asmflags[@]}" "$out/gen/vectors_$name.s" \
    -o "$out/gen/vectors_$name.o"
  "$davecc" "${asmflags[@]}" -I"$paged" "$out/gen/shim_$name.s" \
    -o "$out/gen/shim_$name.o"
  local cross_obj=()
  if [[ "$cross" != none ]]; then
    "$davecc" "${asmflags[@]}" -I"$paged" "$out/gen/cross_$name.s" \
      -o "$out/gen/cross_$name.o"
    cross_obj=("$out/gen/cross_$name.o")
  fi
  "$davecc" -target 6502 -static -nostdlib --gc-sections \
    -Wl,-T -Wl,"$paged/libc_rom.ld" \
    -e paged_rom_entry \
    "$out/gen/rom_header_$name.o" "$out/gen/rom_init_extra.o" \
    "$out/gen/vectors_$name.o" ${cross_obj[@]+"${cross_obj[@]}"} \
    "${rom_objs[@]}" "$@" \
    -o "$out/libc6502_paged_$name.elf"
  shim_members+=("$out/gen/shim_$name.o")
  "$elfdump" -s "$out/libc6502_paged_$name.elf" >"$out/$name.symbols"
  python3 - "$out/$name.symbols" "$name" <<'PY'
import sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read().splitlines()
label = sys.argv[2]
end = None
vectors = None
for line in text:
    parts = line.split()
    if len(parts) < 7:
        continue
    name = parts[-1]
    try:
        value = int(parts[1], 16)
    except ValueError:
        continue
    if name == "__paged_vectors":
        vectors = value
    if name == "_end":
        end = value
if vectors != 0x8100:
    raise SystemExit("%s vectors at %s" % (label, vectors))
if end is None or end > 0xC000:
    raise SystemExit("%s _end %s exceeds the sideways window" % (label, end))
print("%s _end = 0x%x (%d bytes free)" % (label, end, 0xC000 - end))
PY
}

libm_objs=()
for src in acos asin atan atan2 ceil exp fabs fenv floor frexp log pow \
           sincos sqrt tan ldexp modf; do
  libm_objs+=("$out/extra/$src.o")
done
link_extra libm 1 "DaveCC libm" none 0 all "${libm_objs[@]}"

# math_extra is larger than one bank. Its float wrappers only call their
# double versions, so those clusters can be split across two images.
# Each image cross-calls libm for sin, exp, and the rest.
python3 - "$out/extra/math_extra.o" "$paged/exports_math_a.list" \
  "$paged/exports_math_b.list" "$elfdump" <<'PY'
import collections, subprocess, sys
obj, list_a, list_b, elfdump = sys.argv[1:]
def dump(flag):
    return subprocess.check_output([elfdump, flag, obj], text=True, errors="replace")
sizes = {}
for line in dump("-s").splitlines():
    parts = line.split()
    if len(parts) < 7 or parts[3] != "func" or parts[4] != "global":
        continue
    if not parts[5].startswith(".text"):
        continue
    name = parts[-1]
    if name.startswith("__") or name.startswith("."):
        continue
    sizes[name] = int(parts[2])
edges = []
section = ""
for line in dump("-r").splitlines():
    if line.startswith("Section:"):
        section = line.split()[1].rsplit(".", 1)[-1]
        continue
    for tok in line.split():
        if tok in sizes and tok != section:
            edges.append((section, tok))
parent = {}
def find(x):
    parent.setdefault(x, x)
    if parent[x] != x:
        parent[x] = find(parent[x])
    return parent[x]
for name in sizes:
    find(name)
for src, dst in edges:
    if src in sizes and dst in sizes:
        ra, rb = find(src), find(dst)
        if ra != rb:
            parent[rb] = ra
clusters = collections.defaultdict(list)
for name in sizes:
    clusters[find(name)].append(name)
groups = sorted(
    ((sum(sizes[n] for n in members), sorted(members))
     for members in clusters.values()),
    reverse=True)
loads = [0, 0]
bins = [[], []]
for total, members in groups:
    side = 0 if loads[0] <= loads[1] else 1
    bins[side].extend(members)
    loads[side] += total
for path, names in ((list_a, bins[0]), (list_b, bins[1])):
    try:
        existing = open(path, encoding="utf-8").read()
    except FileNotFoundError:
        existing = ""
    if existing.strip():
        continue
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("# Append-only. Do not reorder.\n")
        for name in names:
            handle.write(name + "\n")
PY

link_extra math_a 2 "DaveCC math" "$paged/exports_libm.list" 1 subset \
  "$out/extra/math_extra.o"
link_extra math_b 3 "DaveCC math" "$paged/exports_libm.list" 1 subset \
  "$out/extra/math_extra.o"

cxx_objs=()
for src in iostream iostream_cin iostream_cout iostream_cerr iostream_clog \
           iostream_input cxx_new cxx_new_overloads cxx_guard cxx_rtti \
           exception stdexcept locale bad_cast ctype exit davecc_lifecycle; do
  cxx_objs+=("$out/extra/$src.o")
done
# Subset: exporting every locale method overflows the bank. The list is
# the public entry points. Methods iostream calls stay in the image.
link_extra cxx 4 "DaveCC c++" "$paged/exports.list" 0 subset "${cxx_objs[@]}"

# The float printers do not fit beside the formatter. They live in the
# next image, under names the assembly can call across the gate.
for src in ftoa fpfuncs strtod; do
  "$davecc" "${cflags[@]}" "$root/libc/$src.c" -o "$out/extra/$src.o"
done
"$davecc" "${cflags[@]}" "$paged/float_export.c" -o "$out/extra/float_export.o"
link_extra float 6 "DaveCC float" \
  "0:$paged/exports.list,4:$paged/exports_cxx.list" 0 all \
  "$out/extra/float_export.o" "$out/extra/ftoa.o" "$out/extra/fpfuncs.o" \
  "$out/extra/strtod.o"
"$davecc" "${asmflags[@]}" "$paged/printf.s" -o "$out/extra/printf.o"
link_extra stdio 5 "DaveCC stdio" \
  "0:$paged/exports.list,6:$paged/exports_float.list" 0 all \
  "$out/extra/printf.o"

"$archivist" r "$out/libc6502_paged_shim.a" "${shim_members[@]}"

# Helpers, MOS wrappers, and the syscall object. bbc_start.o is separate
# so the link always has _start. Members stay unmerged so unused helpers
# can be dropped.
user_runtime=()
for obj in "$out/runtime"/*.o; do
  base="$(basename "$obj")"
  if [[ "$base" == "bbc_start.o" ]]; then
    continue
  fi
  user_runtime+=("$obj")
done
"$archivist" r "$out/libc6502_bbc_runtime.a" "${user_runtime[@]}"
cp "$out/runtime/bbc_start.o" "$out/bbc_start.o"
cp "$sup/bbc.ld" "$out/bbc.ld"

python3 - "$out/libc6502_paged.elf" "$out/libc6502_paged.bin" <<'PY'
import struct
import sys

elf_path, bin_path = sys.argv[1], sys.argv[2]
data = open(elf_path, "rb").read()
if data[:4] != b"\x7fELF":
    raise SystemExit("paged libc is not an ELF image")
elf_class = data[4]
image = bytearray(0x4000)
if elf_class == 1:
    e_phoff, = struct.unpack_from("<I", data, 28)
    e_phentsize, e_phnum = struct.unpack_from("<HH", data, 42)
    def load(off):
        p_type, p_offset, p_vaddr, _, p_filesz, p_memsz = struct.unpack_from(
            "<IIIIII", data, off)
        return p_type, p_offset, p_vaddr, p_filesz, p_memsz
elif elf_class == 2:
    e_phoff, = struct.unpack_from("<Q", data, 32)
    e_phentsize, e_phnum = struct.unpack_from("<HH", data, 54)
    def load(off):
        p_type, _, p_offset, p_vaddr, _, p_filesz, p_memsz = struct.unpack_from(
            "<IIQQQQQ", data, off)
        return p_type, p_offset, p_vaddr, p_filesz, p_memsz
else:
    raise SystemExit("unknown ELF class %d" % elf_class)
for i in range(e_phnum):
    p_type, p_offset, p_vaddr, p_filesz, p_memsz = load(e_phoff + i * e_phentsize)
    if p_type != 1:
        continue
    for n in range(p_memsz):
        addr = p_vaddr + n
        if addr < 0x8000 or addr >= 0xC000:
            continue
        value = data[p_offset + n] if n < p_filesz else 0
        image[addr - 0x8000] = value
open(bin_path, "wb").write(image)
print("wrote %s (%d bytes)" % (bin_path, len(image)))
PY

"$elfdump" -s "$out/libc6502_paged.elf" >"$out/rom.symbols"
python3 - "$out/rom.symbols" <<'PY'
import sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read().splitlines()
want = {
    "paged_rom_entry": 0x8000,
    "__paged_vectors": 0x8100,
}
found = {}
end = None
for line in text:
    parts = line.split()
    if len(parts) < 7:
        continue
    name = parts[-1]
    try:
        value = int(parts[1], 16)
    except ValueError:
        continue
    if name in want:
        found[name] = value
    if name == "_end":
        end = value
missing = [name for name in want if name not in found]
if missing:
    raise SystemExit("ROM is missing symbols: %s" % ", ".join(missing))
for name, addr in want.items():
    if found[name] != addr:
        raise SystemExit("%s is at 0x%x, expected 0x%x" % (name, found[name], addr))
if end is None:
    raise SystemExit("ROM has no _end symbol")
if end > 0xC000:
    raise SystemExit("paged libc _end 0x%x exceeds the sideways window" % end)
print("paged libc _end = 0x%x (%d bytes free)" % (end, 0xC000 - end))
PY

for name in libm math_a math_b cxx stdio float; do
  python3 - "$out/libc6502_paged_$name.elf" "$out/libc6502_paged_$name.bin" <<'PY'
import struct
import sys

elf_path, bin_path = sys.argv[1], sys.argv[2]
data = open(elf_path, "rb").read()
if data[:4] != b"\x7fELF":
    raise SystemExit("paged image is not an ELF image")
elf_class = data[4]
image = bytearray(0x4000)
if elf_class == 1:
    e_phoff, = struct.unpack_from("<I", data, 28)
    e_phentsize, e_phnum = struct.unpack_from("<HH", data, 42)
    def load(off):
        p_type, p_offset, p_vaddr, _, p_filesz, p_memsz = struct.unpack_from(
            "<IIIIII", data, off)
        return p_type, p_offset, p_vaddr, p_filesz, p_memsz
elif elf_class == 2:
    e_phoff, = struct.unpack_from("<Q", data, 32)
    e_phentsize, e_phnum = struct.unpack_from("<HH", data, 54)
    def load(off):
        p_type, _, p_offset, p_vaddr, _, p_filesz, p_memsz = struct.unpack_from(
            "<IIQQQQQ", data, off)
        return p_type, p_offset, p_vaddr, p_filesz, p_memsz
else:
    raise SystemExit("unknown ELF class %d" % elf_class)
for i in range(e_phnum):
    p_type, p_offset, p_vaddr, p_filesz, p_memsz = load(e_phoff + i * e_phentsize)
    if p_type != 1:
        continue
    for n in range(p_memsz):
        addr = p_vaddr + n
        if addr < 0x8000 or addr >= 0xC000:
            continue
        value = data[p_offset + n] if n < p_filesz else 0
        image[addr - 0x8000] = value
if image[0] != 0x4C or image[6] != 0x80:
    raise SystemExit("%s is not a service ROM" % bin_path)
open(bin_path, "wb").write(image)
print("wrote %s (%d bytes)" % (bin_path, len(image)))
PY
done

echo "Wrote $out/libc6502_paged.elf and the libm, math, c++, stdio, and float images"
