//
//  printf.c
//  c_compiler
//
//  Created by David Allison on 5/5/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#if 0
#define STATIC static
#else
#define STATIC
#endif

#if 1
// Add a call to this where you want a breakpoint.  Then set a breakpoint in
// Break.
void Break() {}
#endif

// These are in ftoa.c.
extern char* __PrintFloatFormat(double f, int precision, char* buf,
                                size_t size);
extern char* __PrintScientificFormat(double f, int precision, char* buf,
                                     size_t size);
extern char* __PrintGeneralFormat(double f, int precision, char* buf,
                                  size_t size);

// Values for field_width and precision.  Positive numbers
// are specified by user.  Negative numbers below -1 mean
// to use arg -(n - 2).  so *3$ is encoded as -4.
typedef enum {
  kWidthDefault = -1,
  kWidthNextArg = -2,
  kWidthExplicit = 0,
} Width;

typedef enum {
  kModNone = 0,
  kModChar,
  kModShort,
  kModLong,
  kModLongLong,
  kModLongDouble,
  kModIntMax,
  kModSize_t,
  kModPtrdiff_t
} Modifier;

typedef struct {
  Width field_width;
  Width precision;
  bool left_justify;
  bool fill_zero;
  bool prepend_sign;
  bool prepend_space;
  bool alternate_form;
  int fw_argnum;
  int p_argnum;
  Modifier modifier;
  int next_arg_value[2];  // [0]: width, [1]: precision
} ConversionFormat;

// Function to write a string with length to the entity in data.
typedef int (*Writer)(const char* s, size_t len, void* data);

#if 1
STATIC const char* CollectFormat(const char* p, ConversionFormat* format) {
  format->field_width = kWidthDefault;
  format->precision = kWidthDefault;
  format->left_justify = false;
  format->fill_zero = false;
  format->modifier = kModNone;

  int n = 0;
  bool done = false;
  while (!done && *p != '\0') {
    switch (*p) {
      case '-':
        p++;
        format->left_justify = true;
        break;
      case '0':
        if (!format->left_justify) {
          format->fill_zero = true;
        }
        p++;
        break;
      case '#':
        format->alternate_form = true;
        p++;
        break;
      case ' ':
        format->prepend_space = true;
        p++;
        break;
      case '+':
        format->prepend_sign = true;
        p++;
        break;
      default:
        done = true;
        break;
    }
  }
  // Field width.
  if (*p == '*') {
    p++;
    format->field_width = kWidthNextArg;
  } else if (isdigit(*p)) {
    while (isdigit(*p)) {
      n = n * 10 + *p++ - '0';
    }
    format->field_width = n;
  }

  // Precision.
  if (*p == '.') {
    p++;
    if (*p == '*') {
      p++;
      format->precision = kWidthNextArg;
    } else if (isdigit(*p)) {
      n = 0;
      while (isdigit(*p)) {
        n = n * 10 + *p++ - '0';
      }
      format->precision = n;
    }
  }

  // Modifier (length field).
  switch (*p) {
    case 'l':  // l or ll
      if (p[1] == 'l') {
        format->modifier = kModLongLong;
        p++;
      } else {
        format->modifier = kModLong;
      }
      p++;
      break;
    case 'h':  // h or hh
      if (p[1] == 'h') {
        format->modifier = kModChar;
        p++;
      } else {
        format->modifier = kModShort;
      }
      p++;
      break;
    case 'L':
      format->modifier = kModLongDouble;
      p++;
      break;
    case 'z':
      format->modifier = kModSize_t;
      p++;
      break;
    case 'j':
      format->modifier = kModIntMax;
      p++;
      break;
    case 't':
      format->modifier = kModPtrdiff_t;
      p++;
      break;
    default:
      break;
  }

  return p;
}

STATIC void ResolvePrecision(ConversionFormat* fmt, va_list* ap) {
  if (fmt->precision == kWidthNextArg) {
    fmt->precision = va_arg(*ap, int);
  }
}

STATIC void ResolveFieldWidth(ConversionFormat* fmt, va_list* ap) {
  if (fmt->field_width == kWidthNextArg) {
    fmt->field_width = va_arg(*ap, int);
  }
}

STATIC void FixFloatPrecision(ConversionFormat* fmt, size_t max) {
  if (fmt->precision == kWidthDefault) {
    fmt->precision = 6;
  }
  if (fmt->precision <= 0) {
    fmt->precision = 6;
  }
  if (fmt->precision > max) {
    fmt->precision = (int)max;
  }
}

char* ConvertBinary(unsigned long long v, char* buf,
            int buflen) {
  char* p = &buf[buflen - 1];
  if (v == 0) {
    *p = '0';
    return p;
  }
  while (v != 0) {
    char ch = (v & 1) + '0';
    *p-- = ch;
    v >>= 1;
  }
  // p is one less than the first char.
  return p + 1;
}

STATIC char* ConvertDecimalLongLong(unsigned long long v, char* buf,
                                    int buflen) {
  char* p = &buf[buflen - 1];
  if (v == 0) {
    *p = '0';
    return p;
  }
  while (v != 0) {
    lldiv_t qr = lldiv(v, 10);
    char ch = qr.rem + '0';
    *p-- = ch;
    v = qr.quot;
  }
  // p is one less than the first char.
  return p + 1;
}

STATIC char* ConvertHexLongLong(unsigned long long v, char* buf, int buflen,
                                bool upper) {
  char* p = &buf[buflen - 1];
  if (v == 0) {
    *p = '0';
    return p;
  }
  while (v != 0) {
    uint8_t n = v & 0xf;
    char ch;
    if (n > 9) {
      ch = n - 10 + (upper ? 'A' : 'a');
    } else {
      ch = n + '0';
    }
    *p-- = ch;
    v >>= 4;
  }
  return p + 1;
}

STATIC char* ConvertOctalLongLong(unsigned long long v, char* buf, int buflen) {
  char* p = &buf[buflen - 1];
  if (v == 0) {
    *p = '0';
    return p;
  }
  while (v != 0) {
    uint8_t n = v & 0x7;
    char ch = n + '0';
    *p-- = ch;
    v >>= 3;
  }
  return p + 1;
}

STATIC char* ConvertHexPointer(void* ptr, char* buf, int buflen) {
  char* p;
  if (ptr == NULL) {
    // Write (null) (choice - it's up to the implementation what is printed.
    p = &buf[buflen - 6];
    strcpy(p, "(null)");
    return p;
  }
  p = &buf[buflen - 1];
  uintptr_t v = (uintptr_t)ptr;
  while (v != 0) {
    uint8_t n = v & 0xf;
    char ch;
    if (n > 9) {
      ch = n - 10 + 'a';
    } else {
      ch = n + '0';
    }
    *p-- = ch;
    v >>= 4;
  }
  *p-- = 'x';
  *p = '0';
  return p;
}

static const char spaces[] = "        ";
static const char zeroes[] = "00000000";

STATIC int Pad(Writer writer, void* data, int n, bool zero) {
  const size_t kBlockSize = sizeof(spaces) - 1;
  const char* pad = zero ? zeroes : spaces;
  int count = 0;
  while (n > 0) {
    int len = n;
    if (len > kBlockSize) {
      len = kBlockSize;
    }
    writer(pad, len, data);
    n -= len;
    count += len;
  }
  return count;
}

STATIC void Prepend(Writer writer, void* data, ConversionFormat* fmt,
                    bool negative) {
  if (negative) {
    writer("-", 1, data);
    return ;
  }
  if (fmt->prepend_sign) {
    writer("+", 1, data);
    return ;
  }
  if (fmt->prepend_space) {
    writer(" ", 1, data);
    return;
  }
}

// Do we need to prepend a character?
STATIC bool AnyPrependNeeded(ConversionFormat* fmt,
                             bool negative) {
  return negative || fmt->prepend_sign || fmt->prepend_space;
}

STATIC int PadToPrecision(Writer writer, void* data, ConversionFormat* fmt,
                          size_t len, bool enabled) {
  if (!enabled) {
    return 0;
  }
  if (fmt->precision == kWidthDefault) {
    return 0;
  }
  int diff = fmt->precision - (int)len;
  if (diff <= 0) {
    return 0;
  }
  return Pad(writer, data, diff, true);
}

STATIC int WriteFormatted(Writer writer, void* data, ConversionFormat* fmt,
                          const char* s, size_t len, bool negative,
                          bool use_precision) {
  if (fmt->field_width == kWidthDefault) {
    Prepend(writer, data, fmt, negative);
    int num_chars = PadToPrecision(writer, data, fmt, len, use_precision);
    return num_chars + writer(s, len, data);
  }
  
  // There is a field width, pad as necessary.
  int num_chars = 0;
  int padding = fmt->field_width - (int)len;
  if (fmt->left_justify) {
    // Left justify.  Pad to the right with spaces.
    if (AnyPrependNeeded(fmt, negative)) {
      Prepend(writer, data, fmt, negative);
      --padding;
    }
    int n = PadToPrecision(writer, data, fmt, len, use_precision);
    num_chars += n;
    padding -= n;
    num_chars += writer(s, len, data);
    num_chars += Pad(writer, data, padding, false);
  } else {
    // Right justify.  Pad to left with space or '0'.  The sign is
    // prepended to the left if padding with zeroes and after the
    // padding if padding with spaces.
    if (AnyPrependNeeded(fmt, negative)) {
      if (fmt->fill_zero) {
        // Filling with zeroes, add prepend character now.
        Prepend(writer, data, fmt, negative);
      }
      --padding;    // One less padding character now.
    }
    if (use_precision && fmt->precision != kWidthDefault) {
      int diff = fmt->precision - (int)len;
      if (diff > 0) {
        padding -= diff;
      }
    }
    num_chars += Pad(writer, data, padding, fmt->fill_zero);
    if (!fmt->fill_zero) {
      Prepend(writer, data, fmt, negative);
    }
    num_chars +=
        PadToPrecision(writer, data, fmt, len, use_precision);
    num_chars += writer(s, len, data);
  }
  return num_chars;
}

STATIC void RemoveFormatting(ConversionFormat* fmt) {
  fmt->fill_zero = false;
  fmt->prepend_sign = false;
  fmt->prepend_space = false;
}

// Write the value of count into the address specified by p (for %n)
STATIC void WriteCount(ConversionFormat* fmt, int count, void* p) {
  switch (fmt->modifier) {
    case kModLong:
      *(long*)p = count;
      break;
    case kModLongLong:
      *(long long*)p = count;
      break;
    case kModChar:
      *(char*)p = count;
      break;
    case kModShort:
      *(short*)p = count;
      break;
    case kModSize_t:
      *(size_t*)p = count;
      break;
    case kModIntMax:
      *(intmax_t*)p = count;
      break;
    case kModPtrdiff_t:
      *(ptrdiff_t*)p = count;
      break;
    case kModLongDouble:
      *(long double*)p = count;
      break;
    default:
      *(int*)p = count;
      break;
  }
}

#else
STATIC const char* CollectFormat(const char* p, ConversionFormat* format);
STATIC char* ConvertDecimalLongLong(unsigned long long v, char* buf,
                                    int buflen);
STATIC char* ConvertHexLongLong(unsigned long long v, char* buf, int buflen,
                                bool upper);
STATIC char* ConvertHexPointer(void* ptr, char* buf, int buflen);
STATIC bool Prepend(Writer writer, void* data, ConversionFormat* fmt,
                    bool negative, bool suppress_write);
STATIC int WriteFormatted(Writer writer, void* data, ConversionFormat* fmt,
                          const char* s, size_t len, bool negative,
                          bool use_precision);
STATIC void WriteCount(ConversionFormat* fmt, int count, void* p);
#endif

STATIC void GetNextArgument(char cmd, ConversionFormat* fmt, va_list* ap,
                            long long* value_ll, const char** value_s,
                            char* value_c, double* value_f, void** value_p,
                            bool* is_unsigned, bool* negative) {
  switch (cmd) {
    case 'd':
    case 'u':
    case 'i':
    case 'x':
    case 'X':
    case 'o':
      *is_unsigned = cmd == 'u' || cmd == 'x' || cmd == 'X';
      switch (fmt->modifier) {
        case kModLong:
          *value_ll = va_arg(*ap, long);
          break;
        case kModLongLong:
          *value_ll = va_arg(*ap, long long);
          break;
        case kModChar:
          *value_ll = (int)va_arg(*ap, int);
          break;
        case kModShort:
          *value_ll = (int)va_arg(*ap, int);
          break;
        case kModSize_t:
          *value_ll = (int)va_arg(*ap, size_t);
          *is_unsigned = true;
          break;
        case kModIntMax:
          *value_ll = (int)va_arg(*ap, intmax_t);
          break;
        case kModPtrdiff_t:
          *value_ll = (int)va_arg(*ap, ptrdiff_t);
          break;
        case kModLongDouble:
          *value_f = (int)va_arg(*ap, long double);
          break;
       default:
          *value_ll = (int)va_arg(*ap, int);
          break;
      }
      if (!*is_unsigned && *value_ll < 0) {
        *negative = true;
        *value_ll = -*value_ll;
      }
      break;
    case 'p':
    case 'n':
      *value_p = va_arg(*ap, void*);
      break;
    case 'f':
    case 'g':
    case 'e':
      *value_f = va_arg(*ap, double);
      break;
    case 's':
      *value_s = va_arg(*ap, const char*);
      break;
    case 'c':
      *value_c = va_arg(*ap, int);
      break;
    default:
      break;
  }
}

STATIC int Printf(Writer writer, void* data, const char* format, va_list ap) {
  char buf[256];
  char* v;
  const char* end = buf + sizeof(buf);
  const char* p = format;
  int count = 0;
  long long value_ll;
  const char* value_s;
  char value_c;
  double value_f;
  void* value_p;

  while (*p != '\0') {
    if (*p == '%') {
      p++;
      ConversionFormat fmt = {0};
      p = CollectFormat(p, &fmt);
      bool negative = false;
      bool is_unsigned = false;

      // If * is specified for field width or precision, fetch them
      // from the arg list now.
      ResolveFieldWidth(&fmt, &ap);
      ResolvePrecision(&fmt, &ap);

      // Get value from arg list based on conversion.
      GetNextArgument(*p, &fmt, &ap, &value_ll, &value_s, &value_c,
                           &value_f, &value_p, &is_unsigned, &negative);

      // Convert the arg value and write it.
      switch (*p) {
        case 'd':
        case 'i':
        case 'u':
          p++;
          v = ConvertDecimalLongLong(value_ll, buf, sizeof(buf));
          count += WriteFormatted(writer, data, &fmt, v, end - v,
                                  !is_unsigned && negative, true);
          break;
        case 'x':
        case 'X': {
          bool upper = *p == 'X';
          p++;
          v = ConvertHexLongLong(value_ll, buf, sizeof(buf), upper);
          count += WriteFormatted(writer, data, &fmt, v, end - v, false, true);
          break;
        }
        case 'o':
          p++;
          v = ConvertOctalLongLong(value_ll, buf, sizeof(buf));
          count += WriteFormatted(writer, data, &fmt, v, end - v, false, true);
          break;
        case 'p':
          p++;
          v = ConvertHexPointer(value_p, buf, sizeof(buf));
          count += WriteFormatted(writer, data, &fmt, v, end - v, false, true);
          break;
        // TODO: support %a
        case 'f':
          FixFloatPrecision(&fmt, sizeof(buf) - 2);
          p++;
          v = __PrintFloatFormat(value_f, fmt.precision, buf, sizeof(buf));
          count +=
              WriteFormatted(writer, data, &fmt, v, strlen(v), false, false);
          break;
        case 'e':
          FixFloatPrecision(&fmt, sizeof(buf) - 5);
          p++;
          v = __PrintScientificFormat(value_f, fmt.precision, buf, sizeof(buf));
          count +=
              WriteFormatted(writer, data, &fmt, v, strlen(v), false, false);
          break;
        case 'g':
          FixFloatPrecision(&fmt, sizeof(buf) - 5);
          p++;
          v = __PrintGeneralFormat(value_f, fmt.precision, buf, sizeof(buf));
          count +=
              WriteFormatted(writer, data, &fmt, v, strlen(v), false, false);
          break;
        case 'c': {
          p++;
          char b[1] = {value_c};
          // Don't fill or prepend characters.
          RemoveFormatting(&fmt);
          count += WriteFormatted(writer, data, &fmt, b, 1, false, false);
          break;
        }
        case 's': {
          p++;
          // Don't fill or prepend strings.
          RemoveFormatting(&fmt);

          // Limit width to precision.
          size_t len = strlen(value_s);
          if (fmt.precision != kWidthDefault) {
            if (fmt.precision < len) {
              len = fmt.precision;
            }
          }
          count +=
              WriteFormatted(writer, data, &fmt, value_s, len, false, false);
          break;
        }
        case 'n':
          p++;
          WriteCount(&fmt, count, value_p);
          break;
        default:
          count += writer(p++, 1, data);
          break;
      }
    } else {
      count += writer(p++, 1, data);
    }
  }
  return count;
}

#if 1
// Writer function to write to a FILE pointer.
STATIC int FILEWriter(const char* s, size_t len, void* data) {
  FILE* fp = data;
  return fwrite(s, 1, len, fp);
}

typedef struct {
  char* p;      // Current place to write to.
  ssize_t len;  // Remaining space.
} StringData;

// Writer function to write to a string pointer.
STATIC int StringWriter(const char* s, size_t len, void* data) {
  StringData* str = data;
  if (str->len > 0 && len > str->len) {
    len = str->len;
  }
  memcpy(str->p, s, len);
  str->p += len;
  str->len -= len;
  return (int)len;
}

#if !defined(__6502__) && !defined(__risc_v__) && !defined(__aarch64__)
#define fprintf __fprintf
#define printf __printf
#define sprintf __sprintf
#define snprintf __snprintf
#endif

int fprintf(FILE* fp, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Printf(FILEWriter, fp, format, ap);
  va_end(ap);
  return v;
}

int vfprintf(FILE* restrict stream, const char* restrict format, va_list arg) {
  return Printf(FILEWriter, stream, format, arg);
}

int printf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Printf(FILEWriter, stdout, format, ap);
  va_end(ap);
  return v;
}

int vprintf(const char* restrict format, va_list arg) {
  return Printf(FILEWriter, stdout, format, arg);
}

int sprintf(char* s, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, -1};
  int v = Printf(StringWriter, &data, format, ap);
  va_end(ap);
  return v;
}

#if 0
int vsprintf(char* restrict s, const char* restrict format, va_list arg) {
  StringData data = {s, -1};
  return Printf(StringWriter, &data, format, arg);
}
#endif

int snprintf(char* s, size_t len, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, len};
  int v = Printf(StringWriter, &data, format, ap);
  va_end(ap);
  return v;
}

#if 0
int vsnprintf(char* restrict s, size_t n, const char* restrict format,
              va_list arg) {
  StringData data = {s, n};
  return Printf(StringWriter, &data, format, arg);
}
#endif

#endif
