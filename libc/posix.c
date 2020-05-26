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

int errno;

int open(const char* filename, int mode) {
  return syscall(SYS_OPEN, filename, mode);
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

void* malloc(size_t n) {
  void* addr;
  int v = syscall(SYS_MALLOC, n, &addr);
  return v == 0 ? NULL: addr;
}

void free(void* p) {
  syscall(SYS_FREE, (unsigned long)p);
}

void* realloc(void* p, size_t n) {
  void* addr;
  int v = syscall(SYS_REALLOC, p, n, &addr);
  return v == 0 ? NULL : addr;
}

void abort() {
  syscall(SYS_ABORT);
}

