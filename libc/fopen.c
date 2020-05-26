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

FILE* fopen(const char* filename, const char* mode) {
  int open_mode = 0;
  char* m = mode;
  while (*m != '\0') {
    if (*m == 'r') {
      open_mode |= 1;
    } else if (*m == 'w') {
      open_mode |= 2;
    } else if (*m == 'a') {
      open_mode |= 4;
    } else {
      return NULL;
    }
    m++;
  }
  if ((open_mode & 3) == 3) {
    open_mode = O_RDWR;
  } else if ((open_mode & 1) == 1) {
    open_mode = O_RDONLY;
  } else if ((open_mode & 2) == 2) {
      open_mode = O_WRONLY | O_TRUNC;
  } else if ((open_mode & 4) == 4) {
    open_mode = O_WRONLY | O_APPEND;
  }
  int fd = open(filename, open_mode);
  if (fd == -1) {
    return NULL;
  }
  FILE* fp = malloc(sizeof(FILE));
  fp->fd = fd;
  fp->buf = NULL;
  fp->bufsize = 4096;
  fp->index = 0;
  fp->buffering_mode = _IOFBF;    // Fully buffered.
  fp->pos = 0;    // TODO: append should be at end.
  return fp;
}
