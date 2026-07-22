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

void __davecc_stdio_fini(void) {
  fflush(NULL);
}

// setbuf/setvbuf are in stdio_setvbuf.c and feof/ferror/clearerr are in
// stdio_flags.c so this file (pulled in by the stdin/stdout/stderr globals)
// stays minimal.
