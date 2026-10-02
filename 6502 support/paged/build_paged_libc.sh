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
mkdir -p "$out/runtime" "$out/libc" "$out/gen"

cflags=(-target 6502 -c -DDAVECC_BBC -DDAVECC_PAGED_LIBC
        -isystem "$root/libc/include" -I"$root/libc")
asmflags=(-target 6502 -c -I"$sup" -I"$paged" -DLIBC_ROM_SLOT="$slot")

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
# Referencing any libc function pulls the stub member; that member's
# reference to __libc_paged_enter pulls the runtime member, whose strong
# __libc_paged_ensure_init replaces the weak RTS in bbc_start.s.
"$archivist" r "$out/libc6502_paged_shim.a" \
  "$out/gen/shim_funcs.o" "$out/gen/shim_runtime.o"

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

echo "Wrote $out/libc6502_paged.elf and $out/libc6502_paged_shim.a"
