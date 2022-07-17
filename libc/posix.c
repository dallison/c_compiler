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

#if !defined(__6502__)
int errno;
#endif

// The open function can take an extra arg for the open mode
// if O_CREAT is in the flags.
int open(const char* filename, int flags, ...) {
  va_list ap;
  va_start(ap, flags);
  return syscall(SYS_OPEN, filename, flags, va_arg(ap, int));
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
