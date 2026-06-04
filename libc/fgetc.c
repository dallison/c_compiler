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

int getc(FILE* stream) {
  if (stream->buf == NULL) {
    if (stream->error_flag != 0 || stream->eof_flag != 0) {
      return EOF;
    }
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
    return stream->unget_buf[--stream->unget_index];
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
    // Inlined: a nested call would load the stream from the wrong frame slot.
    char* p = stream->buf;
    int remaining = stream->bufsize;
    ssize_t nread_buf[1];
    while (remaining > 0) {
      nread_buf[0] = read(stream->fd, p, remaining);
      if (nread_buf[0] < 1) {
        // EOF/error only if nothing was buffered this fill.
        if (stream->rlimit == stream->rindex) {
          SetErrorOrEof(stream, nread_buf[0]);
        }
        break;
      }
      stream->rlimit += nread_buf[0];
      p += nread_buf[0];
      remaining -= nread_buf[0];
    }
  } else {
    char* p = stream->buf + stream->rlimit;
    ssize_t nread_buf[1];
    for (;;) {
      nread_buf[0] = read(stream->fd, p, 1);
      if (nread_buf[0] <= 0) {
        if (stream->rlimit == stream->rindex) {
          SetErrorOrEof(stream, nread_buf[0]);
        }
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
  if (stream->error_flag != 0 || stream->eof_flag != 0) {
    return EOF;
  }
  return stream->buf[stream->rindex++];
}

int fgetc(FILE* stream) {
  return getc(stream);
}

int getchar(void) {
  return getc(stdin);
}
