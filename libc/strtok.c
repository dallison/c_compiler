//
//  strtok.c
//  c_compiler
//
//  Created by David Allison on 6/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <string.h>
#include <stdbool.h>

char *strtok(char * restrict s1, const char * restrict s2) {
  static char* saved;
  // NULL call, start at 'saved'.
  if (s1 == NULL) {
    s1 = saved;
  }

  // Find first token by searching for first char not in s2.
  size_t len = strlen(s1);
  size_t n = strcspn(s1, s2);
  if (n == len) {
    return NULL;      // No tokens.
  }
  s1 += n;
  len -= n;
  // s1 points to the first non-s2 char.  Look forward until we find
  // the next token.
  n = strspn(s1, s2);
  if (n == len) {
    return s1;
  }
  s1[n] = '\0';
  saved = s1+n+1;
  return s1;
}
