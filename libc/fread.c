//
//  fread.c
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

size_t fread(void * restrict ptr, size_t size, size_t n, FILE* stream) {
  size_t nbytes = size * n;
  int ch;
  size_t nread = 0;
  char* p = ptr;
  while (nbytes > 0 && (ch = fgetc(stream)) != EOF) {
    *p++ = ch;
    nread++;
    nbytes--;
  }
  return nread / size;
}


