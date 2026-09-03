//
//  strings.c
//  c_compiler
//
//  Created by David Allison on 2/14/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int bcmp(const void *a, const void *b, size_t n) { return memcmp(a, b, n); }

void bcopy(const void *src, void *dest, size_t n) { memmove(dest, src, n); }

void bzero(void *p, size_t n) { memset(p, 0, n); }

int ffs(int n) {
  if (n == 0) {
    return 0;
  }
  static const int MultiplyDeBruijnBitPosition2[32] = {
      0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
  n -= (n & (n - 1));
  return MultiplyDeBruijnBitPosition2[(uint32_t)(n * 0x077CB531U) >> 27] + 1;
}

char *index(const char *s, int c) { return strchr(s, c); }

char *rindex(const char *s, int c) { return strrchr(s, c); }

int strcasecmp(const char *a, const char *b) {
  while (*a != '\0' && *b != '\0') {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      break;
    }
    a++;
    b++;
  }
  return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int strncasecmp(const char *a, const char *b, size_t n) {
  while (n != 0 && *a != '\0') {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      break;
    }
    a++;
    b++;
    n--;
  }
  if (n == 0) {
    return 0;
  }
  return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
