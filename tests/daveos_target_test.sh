#!/usr/bin/env bash
# aarch64-unknown-daveos-davecc must define __DAVECC_DAVEOS__ and guest SYS_*,
# and libc wrappers must compile against DaveOS-local syscall numbers.
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
#if !defined(DAVEOS_SYS_FORK) || DAVEOS_SYS_FORK != 200
#error expected DAVEOS_SYS_FORK
#endif
#if SYS_POLL != DAVEOS_SYS_POLL
#error DaveOS SYS_POLL must alias DAVEOS_SYS_POLL
#endif
int daveos_profile_is_valid(void) { return 1; }
SRC

cat >"$WORK/wrappers.c" <<'SRC'
#include <poll.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <davecc_daveos_syscalls.h>

void* keep_daveos_wrappers(void) {
  /* Reference symbols so the front-end type-checks the DaveOS libc API. */
  (void)fork;
  (void)waitpid;
  (void)kill;
  (void)sigaction;
  (void)sigprocmask;
  (void)eventfd;
  (void)timerfd_create;
  (void)timerfd_settime;
  (void)timerfd_gettime;
  (void)epoll_create1;
  (void)epoll_ctl;
  (void)epoll_wait;
  (void)poll;
  (void)select;
  (void)daveos_exec;
  (void)sendfd;
  (void)recvfd;
  (void)sbrk;
  return (void*)1;
}
SRC

"$DAVECC" -target aarch64-unknown-daveos-davecc -std=c11 -S \
  "$WORK/profile.c" -o "$WORK/profile.s"
"$DAVECC" -target aarch64-unknown-daveos-davecc -std=c11 -S \
  "$WORK/wrappers.c" -o "$WORK/wrappers.s"
echo ok
