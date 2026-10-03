//
//  stdio.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define STATIC static

#if defined(DAVECC_PAGED_LIBC)

typedef char paged_file_is_32[(sizeof(FILE) == 32) ? 1 : -1];

static void ClearBytes(unsigned char* p, int n) {
  int i;
  for (i = 0; i < n; i++) {
    p[i] = 0;
  }
}

// Fill the cassette output buffer. The sideways image cannot hold the
// initializers: those addresses are main RAM, and only the primary image
// runs this. A later image's init must not wipe the streams.
void __paged_init_stdio(void) {
  FILE* in = (FILE*)PAGED_STDIN_FILE;
  FILE* out = (FILE*)PAGED_STDOUT_FILE;
  FILE* err = (FILE*)PAGED_STDERR_FILE;
  ClearBytes((unsigned char*)PAGED_STDIN_FILE,
             (PAGED_STDERR_PTR + 2) - PAGED_STDIN_FILE);
  in->fd = 0;
  in->buf = (char*)PAGED_STDIN_BUF;
  in->bufsize = BUFSIZE;
  in->buffering_mode = _IOLBF;
  in->next = out;
  out->fd = 1;
  out->buf = (char*)PAGED_STDOUT_BUF;
  out->bufsize = BUFSIZE;
  out->buffering_mode = _IOLBF;
  out->prev = in;
  out->next = err;
  err->fd = 2;
  err->buffering_mode = _IONBF;
  err->prev = out;
  stdin = in;
  stdout = out;
  stderr = err;
  __all_files = in;
  __last_file = err;
}

FILE* __all_files;
FILE* __last_file;

#else

STATIC char __davecc_stdin_buffer[BUFSIZE];
STATIC char __davecc_stdout_buffer[BUFSIZE];

STATIC FILE __davecc_stdin, __davecc_stdout, __davecc_stderr;
STATIC FILE __davecc_stdin = {.fd = 0,
                              .buf = __davecc_stdin_buffer,
                              .bufsize = BUFSIZE,
                              .buffering_mode = _IOLBF,
                              .prev = NULL,
                              .next = &__davecc_stdout};
STATIC FILE __davecc_stdout = {.fd = 1,
                               .buf = __davecc_stdout_buffer,
                               .bufsize = BUFSIZE,
                               .buffering_mode = _IOLBF,
                               .prev = &__davecc_stdin,
                               .next = &__davecc_stderr};
STATIC FILE __davecc_stderr = {
    .fd = 2, .buffering_mode = _IONBF, .prev = &__davecc_stdout};

FILE* stdin = &__davecc_stdin;
FILE* stdout = &__davecc_stdout;
FILE* stderr = &__davecc_stderr;

FILE* __all_files = &__davecc_stdin;
FILE* __last_file = &__davecc_stderr;

#endif

void __davecc_stdio_fini(void) {
  fflush(NULL);
}

// setbuf/setvbuf are in stdio_setvbuf.c and feof/ferror/clearerr are in
// stdio_flags.c so this file (pulled in by the stdin/stdout/stderr globals)
// stays minimal.
