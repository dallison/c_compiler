//
//  strtoull.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

unsigned long long strtoull(const char* str, const char** end, int base) {
  bool negative = false;
  while (*str != '\0' && isspace(*str)) {
    str++;
  }
  if (*str == '-') {
    negative = true;
    str++;
  } else if (*str == '+') {
    str++;
  }
  const char* start = str;
  if (base == 16 || base == 0) {
    if (str[0] == '0' && tolower(str[1]) == 'x') {
      str += 2;
      base = 16;
    }
  }
  if (base == 8 || base == 0) {
    if (*str == '0') {
      base = 8;
    }
  }
  if (base == 0) {
    base = 10;
  }
  if (base < 2 || base > 36) {
    errno = EINVAL;
    return 0;
  }
  unsigned long long result = 0;
  
  while (*str != '\0') {
    char ch = *str;
    int v = base;
    if (isalpha(ch)) {
      v = toupper(ch) - 'A' + 10;
    } else if (isdigit(ch)) {
      v = ch - '0';
    }
    if (v >= base) {
      break;
    }
    unsigned long long nresult = result * base + v;
    if (nresult < result) {
      // Overflow
      errno = ERANGE;
      return ULLONG_MAX;
    }
    result = nresult;
    str++;
  }
  if (str == start) {
    errno = EINVAL;
    return 0;
  }
  if (end != NULL) {
    *end = str;
  }
  if (negative) {
    result = -result;
  }
  return result;
}
