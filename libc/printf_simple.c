#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Targets of printf specialization when every conversion in the constant
// format is a bare %d, %i, %u, %c, %s or %% (no flags, width, precision or
// length modifier).  Anything else must use a wider profile.

typedef struct {
  FILE* stream;
  char* buffer;
  size_t size;
  size_t used;
  bool failed;
} SimpleOutput;

static void Emit(SimpleOutput* out, char c) {
  if (out->stream != NULL) {
    if (fputc(c, out->stream) == EOF) {
      out->failed = true;
    }
  } else if (out->used + 1 < out->size) {
    out->buffer[out->used] = c;
  }
  out->used++;
}

static int PrintSimple(SimpleOutput* out, const char* format, va_list args) {
  for (; *format != '\0'; format++) {
    char c = *format;
    if (c == '%') {
      c = *++format;
      if (c == 'd' || c == 'i' || c == 'u') {
        unsigned int value;
        if (c == 'u') {
          value = va_arg(args, unsigned int);
        } else {
          int signed_value = va_arg(args, int);
          value = (unsigned int)signed_value;
          if (signed_value < 0) {
            Emit(out, '-');
            value = 0u - value;
          }
        }
        char digits[sizeof(unsigned int) * 3];
        char* p = digits + sizeof(digits);
        do {
          *--p = (char)('0' + value % 10);
          value /= 10;
        } while (value != 0);
        while (p < digits + sizeof(digits)) {
          Emit(out, *p++);
        }
        continue;
      }
      if (c == 's') {
        const char* text = va_arg(args, const char*);
        if (text == NULL) {
          text = "(null)";
        }
        while (*text != '\0') {
          Emit(out, *text++);
        }
        continue;
      }
      if (c == 'c') {
        c = (char)va_arg(args, int);
      }
    }
    Emit(out, c);
  }
  if (out->stream == NULL && out->size != 0) {
    out->buffer[out->used < out->size ? out->used : out->size - 1] = '\0';
  }
  return out->failed ? -1 : (int)out->used;
}

int __printf_simple(const char* format, ...) {
  va_list args;
  va_start(args, format);
  SimpleOutput out = {stdout, NULL, 0, 0, false};
  int result = PrintSimple(&out, format, args);
  va_end(args);
  return result;
}

int __fprintf_simple(FILE* stream, const char* format, ...) {
  va_list args;
  va_start(args, format);
  SimpleOutput out = {stream, NULL, 0, 0, false};
  int result = PrintSimple(&out, format, args);
  va_end(args);
  return result;
}

int __sprintf_simple(char* buffer, const char* format, ...) {
  va_list args;
  va_start(args, format);
  SimpleOutput out = {NULL, buffer, (size_t)-1, 0, false};
  int result = PrintSimple(&out, format, args);
  va_end(args);
  return result;
}

int __snprintf_simple(char* buffer, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  SimpleOutput out = {NULL, buffer, size, 0, false};
  int result = PrintSimple(&out, format, args);
  va_end(args);
  return result;
}
