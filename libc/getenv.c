//
//  getenv.c
//  c_compiler
//
//  Created by David Allison on 6/29/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

// environ is a pointer to an array of pointers to env vars.  Each env
// var is VAR=VALUE or VAR.  The end is a NULL pointer.
char** environ;

char* getenv(const char* var) {
  char** p = environ;
  while (p != NULL) {
    // Get next var.
    char* v = *p++;
    char* s = v;
    // Skip to end of name.
    while (*s != '\0' && *s != '=') {
      s++;
    }
    if (*s == '=') {
      s++;
    }
    size_t len = s - v - 1;
    if (memcmp(var, v, len) == 0) {
      return s;
    }
  }
  return NULL;
}


