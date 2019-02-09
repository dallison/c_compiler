//
//  strcat.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//


#include <stddef.h>

char* strcat(char* dest, const char* src) {
  char* p = dest;
  while (*p != '\0') {
    p++;
  }
  while (*src != '\0') {
    *p++ = *src++;
  }
  *p++ = '\0';
  return dest;
}
