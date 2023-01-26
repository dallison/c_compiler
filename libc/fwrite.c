//
//  fwrite.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

size_t fwrite(const void*  ptr, size_t size, size_t n, FILE* stream) {
  char* p = ptr;
  size_t len = size * n;
  size_t numchars = 0;
  while (len > 0) {
    int v = fputc(*p++, stream);
    if (v == EOF) {
      return n;
    }
    numchars++;
    len--;
  }
  return numchars / size;
}

