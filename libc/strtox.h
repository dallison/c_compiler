#ifndef __davecc_strtox_h
#define __davecc_strtox_h

#include <stdbool.h>
#include <errno.h>

typedef struct {
  unsigned long long value;
  bool negative;
  bool converted;
  bool overflow;
} __StrtoxResult;

static bool __StrtoxSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
         c == '\v';
}

static int __StrtoxDigit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 10;
  }
  return -1;
}

static __StrtoxResult __Strtox(const char* nptr, char** endptr, int base,
                               unsigned long long positive_limit,
                               unsigned long long negative_limit) {
  __StrtoxResult result = {0};
  const char* p = nptr;
  while (__StrtoxSpace(*p)) {
    p++;
  }
  if (*p == '+' || *p == '-') {
    result.negative = *p == '-';
    p++;
  }
  if (base != 0 && (base < 2 || base > 36)) {
    errno = EINVAL;
    if (endptr != NULL) {
      *endptr = (char*)nptr;
    }
    return result;
  }
  bool hexadecimal_prefix =
      (base == 0 || base == 16) && p[0] == '0' &&
      (p[1] == 'x' || p[1] == 'X');
  if (hexadecimal_prefix && __StrtoxDigit(p[2]) >= 0 &&
      __StrtoxDigit(p[2]) < 16) {
    base = 16;
    p += 2;
  } else if (base == 0) {
    base = *p == '0' ? 8 : 10;
  }

  unsigned long long limit =
      result.negative ? negative_limit : positive_limit;
  while (true) {
    int digit = __StrtoxDigit(*p);
    if (digit < 0 || digit >= base) {
      break;
    }
    result.converted = true;
    if (result.value >
        (limit - (unsigned int)digit) / (unsigned int)base) {
      result.overflow = true;
      result.value = limit;
    } else if (!result.overflow) {
      result.value =
          result.value * (unsigned int)base + (unsigned int)digit;
    }
    p++;
  }
  if (endptr != NULL) {
    *endptr = (char*)(result.converted ? p : nptr);
  }
  if (result.overflow) {
    errno = ERANGE;
  }
  return result;
}

#endif
