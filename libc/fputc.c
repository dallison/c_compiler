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


int fputc(char_t c, FILE* stream) {
  if (stream->buf == NULL) {
    char ch = c;
    int e = write(stream->fd, &ch, 1);
    return e == 1 ? c : EOF;
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

