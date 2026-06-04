//
//  fgets.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>

char *fgets(char * restrict s, int n,
            FILE * restrict stream) {
  n--;        // Space for newline.
  char* p = s;
  while (n > 0) {
    int ch = fgetc(stream);
    if (ch == EOF) {
      stream->eof_flag = 1;
      if (p == s) {
        return NULL;
      }
      break;
    }
    *p++ = ch;
    if (ch == '\n') {
      *p = '\0';
      break;
    }
    n--;
  }
  return s;
}

