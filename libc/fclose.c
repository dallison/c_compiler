//
//  fclose.c
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int fclose(FILE* fp) {
  if (fp == NULL) {
    return EOF;
  }
  int e = fflush(fp);
  if (e == EOF) {
    return EOF;
  }
  e = close(fp->fd);
  if (e == -1) {
    return EOF;
  }
  // Unlink from previous.
  if (fp->prev == NULL) {
    __all_files = fp->next;
  } else {
    fp->prev->next = fp->next;
  }
  
  // Unlink from next.
  if (fp->next == NULL) {
    __last_file = fp->prev;
  } else {
    fp->next->prev = fp->prev;
  }

  // If the buffer is owned (set by setvbuf), free it.
  if (fp->buffer_owned) {
    free(fp->buf);
  }
  // For all except standard streams the FILE is allocated on the
  // heap.  Free it.
  if (fp->fd > 2) {
    free(fp);
  }
  return 0;
}
