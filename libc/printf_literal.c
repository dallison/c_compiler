#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int WriteFileLiteral(FILE* stream, const char* text) {
  size_t length = strlen(text);
  return fwrite(text, 1, length, stream) == length ? (int)length : -1;
}

int __printf_literal(const char* format, ...) {
  return WriteFileLiteral(stdout, format);
}

int __fprintf_literal(FILE* stream, const char* format, ...) {
  return WriteFileLiteral(stream, format);
}

int __sprintf_literal(char* buffer, const char* format, ...) {
  size_t length = strlen(format);
  memcpy(buffer, format, length + 1);
  return (int)length;
}

int __snprintf_literal(char* buffer, size_t size, const char* format, ...) {
  size_t length = strlen(format);
  if (size != 0) {
    size_t copy = length < size - 1 ? length : size - 1;
    memcpy(buffer, format, copy);
    buffer[copy] = '\0';
  }
  return (int)length;
}
