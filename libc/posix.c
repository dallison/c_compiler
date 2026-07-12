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

// The open function can take an extra arg for the open mode
// if O_CREAT is in the flags.
#if defined(__p_code__)
int open(const char* filename, int flags, ...) {
  (void)filename;
  (void)flags;
  return -1;
}

int close(int fd) {
  (void)fd;
  return 0;
}

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

int read(int fd, char* buffer, size_t len) {
  (void)fd;
  (void)buffer;
  (void)len;
  return asm(
      "ldw r0, [ap, #16]\n"
      "ldx r1, [ap, #20]\n"
      "ldx r2, [ap, #28]\n"
      "esc #3");
}

long lseek(int fd, fpos_t pos, int whence) {
  (void)fd;
  (void)pos;
  (void)whence;
  return -1;
}

void abort() {
  asm("esc #4");
}

void _Exit(int status) {
  (void)status;
  asm("esc #4");
}
#else
int open(const char* filename, int flags, ...) {
  va_list ap;
  va_start(ap, flags);
  if (flags & O_CREAT) {
    return syscall(SYS_OPEN, filename, flags, va_arg(ap, int));
  }
  return syscall(SYS_OPEN, filename, flags, 0);
}

int close(int fd) {
  return syscall(SYS_CLOSE, fd);
}

int write(int fd, const char* buffer, size_t len) {
  return syscall(SYS_WRITE, fd, buffer, len);
}

int read(int fd, char* buffer, size_t len) {
  return syscall(SYS_READ, fd, buffer, len);
}

long lseek(int fd, fpos_t pos, int whence) {
  return syscall(SYS_LSEEK, fd, pos, whence);
}


void abort() {
  syscall(SYS_ABORT);
}

void _Exit(int status) {
  syscall(SYS_EXIT, status);
}
#endif
