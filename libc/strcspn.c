//
//  strcspn.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <string.h>
#include <stdbool.h>

// Length of initial segment of s1 comprising chars NOT in s2.
size_t strcspn(const char* s1, const char* s2) {
  size_t n = 0;
  while (*s1 != '\0') {
    // Is *s1 in the set of chars in s2?
    bool absent = true;
    for (const char* p = s2; *p != '\0'; p++) {
      if (*s1 == *p) {
        absent = false;
        break;
      }
    }
    if (absent) {
      break;
    }
    n++;
    s1++;
  }
  return n;
}
