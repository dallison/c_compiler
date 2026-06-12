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

int fputc(char_t c, FILE* stream) {
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
    return c;
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
    return fflush(stream);
  }
  return 0;
}

int putchar(char_t c) {
  return fputc(c, stdout);
}

