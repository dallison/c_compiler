//
//  getenv.c
//  c_compiler
//
//  Created by David Allison on 6/29/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syscall.h>

// environ is a pointer to an array of pointers to env vars.  Each env
// var is VAR=VALUE or VAR.  The end is a NULL pointer.
#if defined(__DAVECC_NATIVE_LINUX__) || defined(__6502__)
char** environ;
#endif

#if defined(__DAVECC_NATIVE_LINUX__)
void __davecc_environ_init(char** environment) {
  environ = environment;
}
#endif

char* getenv(const char* var) {
  if (var == NULL || *var == '\0' || strchr(var, '=') != NULL) {
    return NULL;
  }
#if defined(__DAVECC_NATIVE_LINUX__) || defined(__6502__)
  size_t requested_length = strlen(var);
  char** p = environ;
  while (p != NULL && *p != NULL) {
    // Get next var.
    char* v = *p++;
    char* s = v;
    // Skip to end of name.
    while (*s != '\0' && *s != '=') {
      s++;
    }
    size_t name_length = (size_t)(s - v);
    if (*s == '=' && name_length == requested_length &&
        memcmp(var, v, name_length) == 0) {
      return s + 1;
    }
  }
#else
  static char value[4096];
  long result = syscall(SYS_ENVIRONMENT_VALUE, var, value, sizeof(value));
  if (result > 0) return value;
  if (result < 0) errno = (int)-result;
#endif
  return NULL;
}


