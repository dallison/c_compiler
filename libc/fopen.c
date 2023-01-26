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
  const char* m = mode;
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
  if (open_mode == 3) {
    open_mode = O_RDWR;
  } else if (open_mode == 1) {
    open_mode = O_RDONLY;
  } else if (open_mode == 2) {
    open_mode = O_WRONLY | O_TRUNC | O_CREAT;
  } else if (open_mode == 4) {
    open_mode = O_WRONLY | O_APPEND | O_CREAT;
  }
  int fd = open(filename, open_mode, 0777);
  if (fd == -1) {
    return NULL;
  }
  // Allocate the FILE and buffer in one block.
  FILE* fp = malloc(sizeof(FILE) + BUFSIZE);
  if (fp == NULL) {
    return NULL;
  }
  fp->buf = (char*)fp + sizeof(FILE);
  fp->fd = fd;
  fp->bufsize = BUFSIZE;
  fp->windex = 0;
  fp->rindex = fp->bufsize;
  fp->rlimit = 0;
  fp->buffer_owned = 0;       // Buffer doesn't need to be freed.
  fp->buffering_mode = _IOFBF;    // Fully buffered.
  fp->unget_index = 0;
  fp->eof_flag = 0;
  fp->error_flag = 0;
  fp->next = NULL;
  
  // Link into global list of all files.
  __last_file->next = fp;
  fp->prev = __last_file;
  __last_file = fp;
  return fp;
}
