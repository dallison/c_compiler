//
//  fgetc.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

static void SetErrorOrEof(FILE* stream, ssize_t n) {
  if (n < 0) {
    stream->error_flag = 1;
  } else {
    stream->eof_flag = 1;
  }
}

 void ReadFullBuffer(FILE* stream) {
  char* p = stream->buf;
  int remaining = stream->bufsize;
  while (remaining > 0) {
    ssize_t n = read(stream->fd, p, remaining);
    if (n <= 0) {
      SetErrorOrEof(stream, n);
      break;
    }
    stream->rlimit += n;
    p += n;
    remaining -= n;
  }
}

extern void Break();

int getc(FILE* stream) {
  if (stream->buf == NULL) {
    // NON-buffered, call read directly
    char rbuf[1];
    ssize_t n = read(stream->fd, rbuf, 1);
    if (n <= 0) {
      SetErrorOrEof(stream, n);
      return EOF;
    }
    return rbuf[0];
  }
  // Any chars pushed with ungetc?
  if (stream->unget_index > 0) {
    return stream->buf[--stream->unget_index];
  }
  if (stream->rindex < stream->rlimit) {
    // Char available in buffer.
    return stream->buf[stream->rindex++];
  }
  // Nothing available in the buffer.
  if (stream->error_flag != 0 || stream->eof_flag != 0) {
    return EOF;
  }
  stream->rindex = 0;
  stream->rlimit = 0;
  if (stream->buffering_mode == _IOFBF) {
    // Fully buffered, use read for full buffer.
    ReadFullBuffer(stream);
  } else {
    char* p = stream->buf + stream->rlimit;
    for (;;) {
      ssize_t n = read(stream->fd, p, 1);
      if (n <= 0) {
        SetErrorOrEof(stream, n);
        break;
      }
      char ch = *p++;
      stream->rlimit++;
      if (stream->buffering_mode == _IOLBF && ch == '\n') {
        break;
      }
      if (stream->rlimit == stream->bufsize) {
        break;
      }
    }
  }
  return stream->buf[stream->rindex++];
}

int fgetc(FILE* stream) {
  return getc(stream);
}

int getchar() {
  return getc(stdin);
}
