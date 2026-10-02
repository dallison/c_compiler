#!/bin/bash
# Link notepad/bbc_hello.cc for BBC Micro (printf -> SYS_WRITE -> OSWRCH).
# Usage: build_bbc_hello.sh <davecc> <output-elf> [bbc.ld|bbc_mode4.ld]
set -euo pipefail

davecc="$1"
out="$2"
ld="${3:-$(dirname "$0")/bbc.ld}"
root="$(cd "$(dirname "$0")/.." && pwd)"
sup="$(dirname "$0")"
work="$(dirname "$out")/.bbc_hello.build"
rm -rf "$work"
mkdir -p "$work"

runtime=(enter var_addr var_value load_indirect load_indirect8 store_indirect store_indirect8
  push pull_drop frame_push inline_mem_params pushmem copymem zeromem incdec jump_table jmpi
  mul longmul longdiv longlongdiv longlongmul div spill intrinsic
  fadd fmul fdiv fconv fcmp fcommon fneg bbc_start bbc_syscall bbc_mos)
libc=(stdio stdio_flags fputs fwrite fputc fflush exit malloc free memset memcpy strcpy strlen strchr
  printf printf_common printf_literal printf_simple ftoa itoa_int itoa_long itoa_longlong fpfuncs errno
  davecc_lifecycle posix assert_fail)

"$davecc" -target 6502 -std=c++20 -c -isystem "$root/libc/include" "$root/notepad/bbc_hello.cc" -o "$work/hello.o"
for f in "${runtime[@]}"; do
  "$davecc" -target 6502 -c -I "$sup" "$sup/$f.s" -o "$work/$f.o"
done
cflags=(-target 6502 -c -isystem "$root/libc/include" -I"$root/libc")
for f in "${libc[@]}"; do
  "$davecc" "${cflags[@]}" "$root/libc/$f.c" -o "$work/libc_$f.o"
done
link=("$work/hello.o")
for f in "${runtime[@]}"; do link+=("$work/$f.o"); done
for f in "${libc[@]}"; do link+=("$work/libc_$f.o"); done
"$davecc" -target 6502 -static -nostdlib --gc-sections \
  -Wl,-T -Wl,"$ld" \
  "${link[@]}" -o "$out"
echo "Wrote $out"
