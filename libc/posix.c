//
//  posix.c
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <fcntl.h>
#include <syscall.h>
#include <stddef.h>
#include <stdarg.h>

// open/close/read/lseek live in posix_open.c, posix_close.c, posix_read.c
// and posix_lseek.c so they only link when used.  write/abort/_Exit stay
// here; virtually every program needs them.
#if defined(__p_code__)
int write(int fd, const char* buffer, size_t len) {
  (void)fd;
  (void)buffer;
  (void)len;
  return asm(
      "ldw r0, [ap, #16]\n"
      "ldx r1, [ap, #20]\n"
      "ldx r2, [ap, #28]\n"
      "esc #2");
}

__attribute__((noreturn)) void abort() {
  asm("esc #11");
}

void _Exit(int status) {
  (void)status;
  asm(
      "ldw r0, [ap, #16]\n"
      "esc #12");
}
#else
int write(int fd, const char* buffer, size_t len) {
  return syscall(SYS_WRITE, fd, buffer, len);
}

__attribute__((noreturn)) void abort() {
  syscall(SYS_ABORT);
}

void _Exit(int status) {
  syscall(SYS_EXIT, status);
}
#endif
