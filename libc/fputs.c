//
//  fputs.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int fputs(const char* str, FILE* stream) {
  const char* s = str;
  while (*s != '\0') {
    if (fputc(*s++, stream) == EOF) {
      return EOF;
    }
  }
  return 0;
}

int puts(const char* stream) {
  return fputs(stream, stdout);
}

