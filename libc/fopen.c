//
//  fopen.c
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static FILE* AllocateFile(int fd) {
  FILE* fp = malloc(sizeof(FILE) + BUFSIZE);
  if (fp == NULL) {
    return NULL;
  }
  fp->buf = (char*)fp + sizeof(FILE);
  fp->fd = fd;
  fp->bufsize = BUFSIZE;
  fp->windex = 0;
  fp->rindex = 0;
  fp->rlimit = 0;
  fp->buffer_owned = 0;
  fp->buffering_mode = _IOFBF;
  fp->unget_index = 0;
  fp->eof_flag = 0;
  fp->error_flag = 0;
  fp->next = NULL;
  __last_file->next = fp;
  fp->prev = __last_file;
  __last_file = fp;
  return fp;
}

static int ValidMode(const char* mode) {
  char base = '\0';
  for (const char* p = mode; *p != '\0'; p++) {
    if (*p == 'r' || *p == 'w' || *p == 'a') {
      if (base != '\0') return 0;
      base = *p;
    } else if (*p != '+' && *p != 'b') {
      return 0;
    }
  }
  return base != '\0';
}

FILE* fopen(const char* filename, const char* mode) {
  char base = '\0';
  int plus = 0;
  const char* m = mode;
  while (*m != '\0') {
    if (*m == 'r' || *m == 'w' || *m == 'a') {
      base = *m;
    } else if (*m == '+') {
      plus = 1;
    } else if (*m == 'b') {
      // Text/binary distinction is a no-op on this platform.
    } else {
      return NULL;
    }
    m++;
  }
  int open_mode;
  if (base == 'r') {
    open_mode = plus ? O_RDWR : O_RDONLY;
  } else if (base == 'w') {
    open_mode = (plus ? O_RDWR : O_WRONLY) | O_TRUNC | O_CREAT;
  } else if (base == 'a') {
    open_mode = (plus ? O_RDWR : O_WRONLY) | O_APPEND | O_CREAT;
  } else {
    return NULL;
  }
  int fd = open(filename, open_mode, 0777);
  if (fd == -1) {
    return NULL;
  }
  FILE* fp = AllocateFile(fd);
  if (fp == NULL) {
    close(fd);
    return NULL;
  }
  return fp;
}

FILE* fdopen(int fd, const char* mode) {
  if (fd < 0 || mode == NULL || !ValidMode(mode)) {
    return NULL;
  }
  return AllocateFile(fd);
}
