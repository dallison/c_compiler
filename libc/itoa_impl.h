// Template body for the __itoa/__utoa conversion functions.  Include this
// from a translation unit after defining:
//   ITOA_UTYPE - the unsigned type to convert
//   ITOA_STYPE - the matching signed type
//   ITOA_UNAME - the name of the unsigned conversion function
//   ITOA_SNAME - the name of the signed conversion function
// Output matches snprintf's %u/%x/%X/%o (plus %#x/%#o prefixes) so callers
// migrating from printf-based formatting see identical text.

#include <__itoa.h>
#include <stddef.h>

size_t ITOA_UNAME(char* buffer, ITOA_UTYPE value, unsigned char base,
                  unsigned char flags) {
  // Enough for the octal expansion of the widest value.
  char digits[sizeof(ITOA_UTYPE) * 8 / 3 + 2];
  char* end = digits + sizeof(digits);
  char* p = end;
  char letter_base = (flags & __DAVECC_ITOA_UPPER) ? 'A' : 'a';
  ITOA_UTYPE remaining = value;
  do {
    unsigned char digit = (unsigned char)(remaining % base);
    remaining /= base;
    *--p = digit < 10 ? (char)('0' + digit)
                      : (char)(letter_base + (digit - 10));
  } while (remaining != 0);

  char* out = buffer;
  if (flags & __DAVECC_ITOA_NEGATIVE) {
    *out++ = '-';
  } else if (flags & __DAVECC_ITOA_SHOWPOS) {
    *out++ = '+';
  }
  // Like %#x / %#o, zero gets no base prefix.
  if ((flags & __DAVECC_ITOA_SHOWBASE) && value != 0) {
    if (base == 16) {
      *out++ = '0';
      *out++ = (flags & __DAVECC_ITOA_UPPER) ? 'X' : 'x';
    } else if (base == 8) {
      *out++ = '0';
    }
  }
  while (p != end) {
    *out++ = *p++;
  }
  return (size_t)(out - buffer);
}

size_t ITOA_SNAME(char* buffer, ITOA_STYPE value, unsigned char base,
                  unsigned char flags) {
  ITOA_UTYPE magnitude = (ITOA_UTYPE)value;
  if (base == 10 && value < 0) {
    magnitude = (ITOA_UTYPE)(-(value + 1)) + 1u;
    flags = (unsigned char)((flags | __DAVECC_ITOA_NEGATIVE) &
                            ~__DAVECC_ITOA_SHOWPOS);
  }
  return ITOA_UNAME(buffer, magnitude, base, flags);
}
