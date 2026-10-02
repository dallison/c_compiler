#!/bin/bash
# Link notepad/sine_fp.cc for BBC Micro / Econet (*RUN loads PT_LOAD ELF).
# Match notepad/sine.cc: bbc_mode4.ld @ &3000, MOS print, only the libc math sin needs.
# Usage: build_bbc_sine_fp.sh [--printf|--plot] <davecc> <output-elf> [linker-script]
# --printf keeps the printf output path; that image only fits the mode 7
# layout, so it defaults to bbc_mode7.ld.
# --plot builds notepad/sine_fp_plot.cc, which draws the curve in mode 4.
set -euo pipefail

use_printf=0
source_name=sine_fp
if [[ "${1:-}" == "--printf" ]]; then
  use_printf=1
  shift
elif [[ "${1:-}" == "--plot" ]]; then
  source_name=sine_fp_plot
  shift
fi
davecc="$1"
out="$2"
if [[ $use_printf -eq 1 ]]; then
  ld="${3:-$(dirname "$0")/bbc_mode7.ld}"
else
  ld="${3:-$(dirname "$0")/bbc_mode4.ld}"
fi
root="$(cd "$(dirname "$0")/.." && pwd)"
sup="$(dirname "$0")"
work="$(dirname "$out")/.bbc_sine_fp.build"
rm -rf "$work"
mkdir -p "$work"

runtime=(enter var_addr var_value load_indirect load_indirect8 store_indirect store_indirect8
  push pull_drop frame_push inline_mem_params pushmem copymem zeromem incdec jump_table jmpi
  mul longmul longdiv longlongdiv longlongmul div spill intrinsic
  fadd fmul fdiv fconv fcmp fcommon fneg bbc_start bbc_syscall bbc_mos)
if [[ $source_name == sine_fp_plot ]]; then
  runtime+=(bbc_vdu)
fi
bbc_cflags=(-target 6502 -std=c++20 -c -isystem "$root/libc/include")

libc_srcs=(sincos modf)
if [[ $use_printf -eq 1 ]]; then
  libc_srcs+=(stdio stdio_flags fputs fwrite fputc fflush exit malloc free
    memset memcpy strcpy strlen strchr printf printf_common printf_literal
    printf_simple ftoa itoa_int itoa_long itoa_longlong fpfuncs errno
    davecc_lifecycle posix assert_fail)
else
  bbc_cflags+=(-DSINE_FP_BBC)
fi

"$davecc" "${bbc_cflags[@]}" "$root/notepad/$source_name.cc" -o "$work/sine_fp.o"
for f in "${libc_srcs[@]}"; do
  "$davecc" -target 6502 -c -isystem "$root/libc/include" -I "$root/libc" \
    "$root/libc/$f.c" -o "$work/libc_$f.o"
done
for f in "${runtime[@]}"; do
  "$davecc" -target 6502 -c -I "$sup" "$sup/$f.s" -o "$work/$f.o"
done
link=("$work/sine_fp.o")
for f in "${libc_srcs[@]}"; do link+=("$work/libc_$f.o"); done
for f in "${runtime[@]}"; do link+=("$work/$f.o"); done
"$davecc" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$ld" \
  "${link[@]}" -o "$out"

python3 - "$out" <<'PY'
import struct, sys
path = sys.argv[1]
b = open(path, "rb").read()
entry = struct.unpack_from("<Q", b, 24)[0]
phoff = struct.unpack_from("<Q", b, 32)[0]
phentsize, phnum = struct.unpack_from("<HH", b, 54)
base = None
end = 0
for i in range(phnum):
    ph = b[phoff + i * phentsize :]
    typ = struct.unpack_from("<I", ph, 0)[0]
    if typ != 1:
        continue
    off, vaddr, _paddr, filesz, memsz = struct.unpack_from("<QQQQQ", ph, 8)
    if memsz == 0:
        continue
    if filesz > memsz:
        filesz = memsz
    seg_end = vaddr + memsz
    base = vaddr if base is None else min(base, vaddr)
    end = max(end, seg_end)
print(f"Wrote {path} ({len(b)} bytes) Econet load={base:#x} exec={entry:#x} span={end - base} bytes")
if base is None or not (base <= entry < end):
    sys.exit("ELF entry point outside PT_LOAD span")
PY
