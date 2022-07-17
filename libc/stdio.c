//
//  stdio.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#if 0
#define STATIC static
#else
#define STATIC 
#endif

STATIC char s_stdin_buf[BUFSIZE];
STATIC char s_stdout_buf[BUFSIZE];

STATIC FILE s_stdin = {.fd = 0, .buf = s_stdin_buf, .bufsize = BUFSIZE,
  .rindex = 0, .rlimit = 0, .windex = 0, .buffering_mode = _IOLBF, .buffer_owned = 0};
STATIC FILE s_stdout = {.fd = 1, .buf = s_stdout_buf, .bufsize = BUFSIZE, .rindex = 0, .rlimit = 0,
  .windex = 0, .buffering_mode = _IOLBF};
STATIC FILE s_stderr = {.fd = 2, .buf = NULL, .bufsize = 0,
  .rindex = 0, .rlimit = 0, .windex = 0, .buffering_mode =_IONBF};

FILE* stdin = &s_stdin;
FILE* stdout = &s_stdout;
FILE* stderr = &s_stderr;

void setbuf(FILE * restrict stream,
            char * restrict buf) {
  setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

int setvbuf(FILE * restrict stream,
     char * restrict buf,
            mode_t mode, size_t size) {
  if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
    return -1;
  }
  if (stream->buf != NULL && stream->buffer_owned) {
    free(stream->buf);
  }
  if (buf == NULL && mode != _IONBF) {
    stream->buf = malloc(size);
    stream->buffer_owned = 1;
  } else {
    stream->buf = buf;
  }
  stream->bufsize = size;
  stream->buffering_mode = mode;
  return 0;
}

int feof(FILE* stream) {
  return stream->eof_flag;
}

int ferror(FILE* stream) {
  return stream->error_flag;
}

void clearerr(FILE *stream) {
  stream->error_flag = 0;
}
