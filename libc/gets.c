//
//  gets.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>

char* gets(char *str) {
  char* p = str;
  for (;;) {
    char ch = fgetc(stdin);
    if (ch == EOF) {
      stdin->eof_flag = 1;
      if (p == str) {
        return NULL;
      }
      break;
    }
    *p++ = ch;
    if (ch == '\n') {
      p[-1] = '\0';
      break;
    }
  }
  return str;
}
