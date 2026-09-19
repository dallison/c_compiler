//
//  syscall.c
//  c_compiler
//
//  Created by David Allison on 2/15/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdarg.h>
#include <stddef.h>
#include <syscall.h>
#include <time.h>

#if defined(__DAVECC_NATIVE_LINUX__)
typedef struct {
#if defined(__risc_v__) && defined(__ILP32__)
  long long tv_sec;
  long long tv_nsec;
#else
  long tv_sec;
  long tv_nsec;
#endif
} DaveLinuxTimespec;

time_t time(time_t* result) {
  DaveLinuxTimespec value;
  if (syscall(
#if defined(__risc_v__) && defined(__ILP32__)
          SYS_clock_gettime64,
#else
          SYS_clock_gettime,
#endif
          0, &value) < 0) {
    return (time_t)-1;
  }
  if ((time_t)value.tv_sec != value.tv_sec) {
    return (time_t)-1;
  }
  if (result != NULL) {
    *result = (time_t)value.tv_sec;
  }
  return (time_t)value.tv_sec;
}

clock_t clock(void) {
  DaveLinuxTimespec value;
  if (syscall(
#if defined(__risc_v__) && defined(__ILP32__)
          SYS_clock_gettime64,
#else
          SYS_clock_gettime,
#endif
          2, &value) < 0) {
    return (clock_t)-1;
  }
  return (clock_t)(value.tv_sec * CLOCKS_PER_SEC +
                   value.tv_nsec / (1000000000 / CLOCKS_PER_SEC));
}
#else
time_t time(time_t* result) {
  time_t value = (time_t)syscall(SYS_TIME);
  if (result != NULL) {
    *result = value;
  }
  return value;
}

clock_t clock(void) {
  return (clock_t)syscall(SYS_CLOCK);
}
#endif

#if defined(__DAVECC_NATIVE_LINUX__)
// Implemented in the architecture-specific Linux runtime.
#elif defined(__risc_v__)
// Args are:
// a0: syscall number
// a1...: args to syscall
long syscall(int n, ...) {
   return asm(
              "mv t6, a0\n"
              "ecall"
              );
}
#elif defined(__p_code__)
long syscall(int n, ...) {
  (void)n;
  return asm(
      "pushx r1\n"
      "ldw r0, [ap, #16]\n"
      "addc r1, ap, #20\n"
      "esc #7\n"
      "popx r1");
}
#elif defined(__x86_64__)
// Implemented in x86_64 support/syscall.s
#elif defined(__i386__)
// Implemented in x86 support/syscall.s
#elif defined(__aarch64__)
// Implemented in aarch64 support/syscall.s
#elif defined(__arm__)
// Implemented in arm support/syscall.s
#elif defined(__xtensa__)
// Implemented in xtensa_support/syscall.s
#elif defined(__6502__)
// Implemented in 6502runtime.s
#elif defined(__wasm32__)
// Implemented in wasi_syscall.c, since a wasm call out of the module is an
// ordinary call rather than anything that could be written here.
#else
#error "Unknown architecture"
#endif
