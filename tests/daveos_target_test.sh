#!/usr/bin/env bash
# aarch64-unknown-daveos-davecc must define __DAVECC_DAVEOS__ and guest SYS_*.
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/daveos-target.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

cat >"$WORK/profile.c" <<'SRC'
#include <syscall.h>
#ifndef __DAVECC_DAVEOS__
#error expected __DAVECC_DAVEOS__
#endif
#ifdef __DAVECC_NATIVE_LINUX__
#error DaveOS must not set NATIVE_LINUX
#endif
#ifdef __DAVECC_INTERPRETER_ABI__
#error DaveOS must not set INTERPRETER_ABI
#endif
#ifndef __linux__
#error DaveOS reuses Linux userspace header macros
#endif
#if SYS_WRITE != 4 || SYS_THREAD_YIELD != 23 || SYS_THREAD_SLEEP != 28
#error unexpected Dave guest syscall numbers
#endif
int daveos_profile_is_valid(void) { return 1; }
SRC

"$DAVECC" -target aarch64-unknown-daveos-davecc -std=c11 -S \
  "$WORK/profile.c" -o "$WORK/profile.s"
echo ok
