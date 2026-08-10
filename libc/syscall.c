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

#if defined(__risc_v__)
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
      "ldw r0, [ap, #16]\n"
      "addc r1, ap, #20\n"
      "esc #7");
}
#elif defined(__x86_64__)
// Implemented in x86_64 support/syscall.s
#elif defined(__aarch64__)
// Implemented in aarch64 support/syscall.s
#elif defined(__arm__)
// Implemented in arm support/syscall.s
#elif defined(__6502__)
// Implemented in 6502runtime.s
#else
#error "Unknown architecture"
#endif
