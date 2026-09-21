#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/native-linux-target.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

compile_profile() {
  local target=$1
  local read_number=$2
  local openat_number=$3
  local clone_number=$4
  local source="$WORK/${target%%-*}.c"
  cat >"$source" <<SRC
#include <sys/syscall.h>
#ifndef __linux__
#error Linux target must define __linux__
#endif
#ifndef __unix__
#error Linux target must define __unix__
#endif
#ifndef __DAVECC_NATIVE_LINUX__
#error Linux target must define __DAVECC_NATIVE_LINUX__
#endif
#ifdef __DAVECC_INTERPRETER_ABI__
#error Linux target must not define the interpreter ABI
#endif
#if __NR_read != $read_number || SYS_read != $read_number
#error unexpected read syscall number
#endif
#if __NR_openat != $openat_number || SYS_openat != $openat_number
#error unexpected openat syscall number
#endif
#if __NR_clone != $clone_number || SYS_clone != $clone_number
#error unexpected clone syscall number
#endif
int target_profile_is_valid(void) { return 1; }
SRC
  "$DAVECC" -target "$target" -std=c11 -S "$source" \
    -o "$WORK/${target%%-*}.s"
}

compile_profile aarch64-unknown-linux-davecc 63 56 220
compile_profile x86_64-unknown-linux-davecc 0 257 56
compile_profile arm-unknown-linux-davecc 3 322 120
compile_profile riscv-unknown-linux-davecc 63 56 220
compile_profile riscv32-unknown-linux-davecc 63 56 220

cat >"$WORK/interpreter.c" <<'SRC'
#ifdef __linux__
#error bare targets must not claim Linux
#endif
#ifdef __DAVECC_NATIVE_LINUX__
#error bare targets must not claim the native Linux ABI
#endif
#ifndef __DAVECC_INTERPRETER_ABI__
#error bare targets must retain the interpreter ABI
#endif
int interpreter_profile_is_valid(void) { return 1; }
SRC
"$DAVECC" -target aarch64 -std=c11 -S "$WORK/interpreter.c" \
  -o "$WORK/interpreter.s"

if "$DAVECC" -target aarch64-unknown-notlinux-davecc -std=c11 -S \
    "$WORK/interpreter.c" -o "$WORK/invalid.s" \
    >"$WORK/invalid.out" 2>&1; then
  echo "invalid target OS was accepted" >&2
  exit 1
fi
if [[ "$(<"$WORK/invalid.out")" != *"unsupported target OS"* ]]; then
  echo "invalid target OS did not produce the expected diagnostic" >&2
  exit 1
fi

if "$DAVECC" -target aarch64-unknown-linux-gnu -std=c11 -S \
    "$WORK/interpreter.c" -o "$WORK/invalid-environment.s" \
    >"$WORK/invalid-environment.out" 2>&1; then
  echo "invalid target environment was accepted" >&2
  exit 1
fi
if [[ "$(<"$WORK/invalid-environment.out")" != \
      *"unsupported target environment"* ]]; then
  echo "invalid target environment did not produce the expected diagnostic" >&2
  exit 1
fi

# On a Linux host, omitting -target and passing -fnative must select the
# hosted Linux profile (same macros as ARCH-unknown-linux-davecc).
if [[ "$(uname -s)" == "Linux" ]]; then
  cat >"$WORK/linux_host.c" <<'SRC'
#ifndef __linux__
#error Linux host default must define __linux__
#endif
#ifndef __DAVECC_NATIVE_LINUX__
#error Linux host default must define __DAVECC_NATIVE_LINUX__
#endif
#ifdef __DAVECC_INTERPRETER_ABI__
#error Linux host default must not define the interpreter ABI
#endif
int linux_host_profile_is_valid(void) { return 1; }
SRC
  "$DAVECC" -std=c11 -S "$WORK/linux_host.c" -o "$WORK/linux_omitted.s"
  "$DAVECC" -fnative -std=c11 -S "$WORK/linux_host.c" -o "$WORK/linux_fnative.s"
  host_arch=$(uname -m)
  case "$host_arch" in
    x86_64|amd64) host_bare=x86_64 ;;
    aarch64|arm64) host_bare=aarch64 ;;
    armv7*|armv6*|arm) host_bare=arm ;;
    riscv64) host_bare=riscv ;;
    riscv32) host_bare=riscv32 ;;
    *) host_bare= ;;
  esac
  if [[ -n "$host_bare" ]]; then
    "$DAVECC" -fnative -target "$host_bare" -std=c11 -S \
      "$WORK/linux_host.c" -o "$WORK/linux_fnative_bare.s"
  fi

  cat >"$WORK/wrong_arch.c" <<'SRC'
int wrong_arch(void) { return 0; }
SRC
  if [[ "$host_bare" == "x86_64" ]]; then
    if "$DAVECC" -fnative -target aarch64 -nostdinc -nostdlib -c \
        "$WORK/wrong_arch.c" -o "$WORK/wrong_arch.o" \
        >"$WORK/wrong_arch.out" 2>&1; then
      echo "-fnative accepted a non-host architecture" >&2
      exit 1
    fi
    if [[ "$(<"$WORK/wrong_arch.out")" != *"host Linux architecture"* ]]; then
      echo "-fnative wrong-arch diagnostic was missing" >&2
      cat "$WORK/wrong_arch.out" >&2
      exit 1
    fi
  fi
fi
