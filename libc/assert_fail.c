// Assertion failure reporting for the assert() macro.
//
// This deliberately avoids printf: routing the message through printf would
// pull the whole formatting machinery into any program that links malloc (or
// anything else using assert).  It writes directly to fd 2 instead.

#include <__itoa.h>
#include <stdlib.h>
#include <string.h>

int write(int fd, const char* buffer, size_t len);

static void WriteString(const char* text) {
  write(2, text, strlen(text));
}

void __davecc_assert_fail(const char* expression, const char* file, int line) {
  char digits[__DAVECC_ITOA_CAPACITY(int)];
  WriteString(file);
  WriteString(":");
  write(2, digits, __itoa_int(digits, line, 10, 0));
  WriteString(": failed assertion `");
  WriteString(expression);
  WriteString("'\n");
  abort();
}
