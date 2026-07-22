#include <__itoa.h>
#include <stddef.h>
#include <stdint.h>

// Formats like %p: "0x" followed by lowercase hex digits (NULL is "0x0").
size_t __ptrtoa(char* buffer, const void* value) {
  char digits[sizeof(uintptr_t) * 2];
  char* end = digits + sizeof(digits);
  char* p = end;
  uintptr_t remaining = (uintptr_t)value;
  do {
    unsigned char digit = (unsigned char)(remaining & 15);
    remaining >>= 4;
    *--p = digit < 10 ? (char)('0' + digit) : (char)('a' + (digit - 10));
  } while (remaining != 0);

  char* out = buffer;
  *out++ = '0';
  *out++ = 'x';
  while (p != end) {
    *out++ = *p++;
  }
  return (size_t)(out - buffer);
}
