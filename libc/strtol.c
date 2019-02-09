//
//  strtol.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>

extern int errno;

#define EINVAL 22

long long strtoll(const char* str, const char** end, int base) {
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
  long long result = 0;
  
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
    result = result * base + v;
    str++;
  }
  if (end != NULL) {
    *end = str;
  }
  if (negative) {
    result = -result;
  }
  return result;
}

long strtol(const char* str, const char** end, int base) {
  return (long)strtoll(str, end, base);
}

int atoi(const char* s) {
  return (int)strtol(s, NULL, 10);
}
