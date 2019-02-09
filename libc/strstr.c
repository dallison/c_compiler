//
//  strstr.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>

char* strstr(const char* haystack, const char* needle) {
  const char* p = haystack;
  const char* s = needle;
  while (*p != '\0') {
    const char* start = p;
    while (*p++ == *s && *s != '\0') {
      s++;
    }
    if (*s == '\0') {
      return (char*)start;
    }
    s = needle;
  }
  return NULL;
}
