//
//  syscall.c
//  c_compiler
//
//  Created by David Allison on 2/15/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include <time.h>

#if defined(__DAVECC_INTERPRETER_ABI__) && defined(__linux__)
#undef syscall

struct DaveccLinuxMmapRequest {
  uint64_t address;
  uint64_t length;
  int32_t protection;
  int32_t flags;
  int32_t fd;
  int32_t pad;
  int64_t offset;
};

long __davecc_linux_syscall(long number, ...) {
  unsigned long arg[6];
  va_list ap;
  va_start(ap, number);
  for (int i = 0; i < 6; i++) arg[i] = va_arg(ap, unsigned long);
  va_end(ap);
  if (number == SYS_read) return syscall(SYS_READ, arg[0], arg[1], arg[2]);
  if (number == SYS_write) return syscall(SYS_WRITE, arg[0], arg[1], arg[2]);
  if (number == SYS_close) return syscall(SYS_CLOSE, arg[0]);
  if (number == SYS_munmap) return syscall(SYS_MUNMAP, arg[0], arg[1]);
  if (number == SYS_mprotect) {
    return syscall(SYS_MPROTECT, arg[0], arg[1], arg[2]);
  }
  if (number == SYS_getpid) return syscall(SYS_GETPID);
  if (number == SYS_mmap || number == SYS_mmap2) {
    struct DaveccLinuxMmapRequest request;
    request.address = (uint64_t)arg[0];
    request.length = (uint64_t)arg[1];
    request.protection = (int32_t)arg[2];
    request.flags = (int32_t)arg[3];
    request.fd = (int32_t)arg[4];
    request.pad = 0;
    request.offset = number == SYS_mmap2 ? (int64_t)arg[5] * 4096
                                         : (int64_t)arg[5];
    return syscall(SYS_MMAP, &request);
  }
  errno = ENOSYS;
  return -1;
}
#endif

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
