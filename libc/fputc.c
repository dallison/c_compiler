//
//  fputc.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

extern void Break();
extern void (*__davecc_stdio_fini_hook)(void);
void __davecc_stdio_fini(void);

int fputc(int c, FILE* stream) {
  if (stream->orientation == 0) {
    stream->orientation = -1;
  }
  if (stream->buf == NULL) {
    char ch = c;
    ssize_t remaining = 1;
    while (remaining > 0) {
      ssize_t n = write(stream->fd, &ch, 1);
      if (n < 0) {
        return EOF;
      }
      remaining -= n;
    }
    return (unsigned char)c;
  }
  if (__davecc_stdio_fini_hook == NULL) {
    __davecc_stdio_fini_hook = __davecc_stdio_fini;
  }
  // Buffer full?
  if (stream->windex == stream->bufsize) {
    int e = fflush(stream);
    if (e != 0) {
      return e;
    }
  }
  // Add to next position in buffer.
  stream->buf[stream->windex++] = c;

  if (c == '\n' && stream->buffering_mode == _IOLBF) {
    // Flush on newline.
    if (fflush(stream) != 0) {
      return EOF;
    }
  }
  return (unsigned char)c;
}

int putchar(int c) {
  return fputc(c, stdout);
}

