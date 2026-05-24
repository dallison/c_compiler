#!/bin/bash
# Compile every libc translation unit for the given target.
set -uo pipefail

if [ "$#" -lt 2 ]; then
  echo "usage: $0 <davecc> <target> [extra davecc flags...]" >&2
  exit 2
fi

davecc=$1
target=$2
shift 2

work=$(mktemp -d "${TMPDIR:-/tmp}/libc_compile_test.XXXXXX")
trap 'rm -rf "$work"' EXIT

case "$target" in
  x86_64)
    cflags=(-target "$target" -O1 -c -isystem libc/include -Ilibc "$@")
    ;;
  6502|65c02)
    # Match the flags used by //:libc_65c02 in BUILD.bazel.
    cflags=(-target "$target" -c -isystem libc/include -Ilibc -I"6502 support" "$@")
    ;;
  riscv|risc-v)
    cflags=(-target "$target" -O1 -c -isystem libc/include -Ilibc "$@")
    ;;
  *)
    cflags=(-target "$target" -O1 -c -isystem libc/include -Ilibc "$@")
    ;;
esac

runtime_srcs=(
  "x86_64 support/setjmp.s"
  "x86_64 support/longjmp.s"
  "x86_64 support/syscall.s"
  "x86_64 support/abs.s"
)

pass=0
fail=0
skip=0

compile_one() {
  local src=$1
  local obj=$2
  if "$davecc" "${cflags[@]}" "$src" -o "$obj" 2>"$work/err.txt"; then
    pass=$((pass + 1))
    return 0
  fi
  local status=$?
  if [ "$status" -eq 139 ]; then
    echo "SEGV $src"
  else
    echo "FAIL $src"
    sed 's/^/  /' "$work/err.txt"
  fi
  fail=$((fail + 1))
  return 1
}

if [ "$target" = "x86_64" ]; then
  for src in "${runtime_srcs[@]}"; do
    obj="$work/$(basename "${src%.s}").o"
    compile_one "$src" "$obj" || true
  done
fi

for src in libc/*.c; do
  base=$(basename "$src")
  case "$base" in
    abs.c)
      if [ "$target" = "x86_64" ]; then
        skip=$((skip + 1))
        echo "SKIP $src (provided by x86_64 support/abs.s)"
        continue
      fi
      ;;
    syscall.c)
      if [ "$target" = "6502" ] || [ "$target" = "65c02" ]; then
        skip=$((skip + 1))
        echo "SKIP $src (6502 libc uses 6502 support/syscall.s)"
        continue
      fi
      ;;
    posix.c)
      if [ "$target" = "6502" ]; then
        skip=$((skip + 1))
        echo "SKIP $src (requires 65c02 target for syscall.h)"
        continue
      fi
      ;;
  esac
  obj="$work/${base%.c}.o"
  compile_one "$src" "$obj" || true
done

echo "compile summary: pass=$pass fail=$fail skip=$skip"
if [ "$fail" -ne 0 ]; then
  exit 1
fi
