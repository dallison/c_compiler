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

#define STATIC static

#if 1
// Add a call to this where you want a breakpoint.  Then set a breakpoint in
// Break.
STATIC void Break() {}
#endif

#ifndef PRINTF_DISABLE_FLOAT
// These are in ftoa.c.
extern char* __PrintFloatFormat(double f, int precision, char* buf,
                                size_t size);
extern char* __PrintScientificFormat(double f, int precision, char* buf,
                                     size_t size);
extern char* __PrintGeneralFormat(double f, int precision, char* buf,
                                  size_t size);
#endif

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
  kModPtrdiff_t,
  kModWidth,
  kModWidthFast
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
  int modifier_width;
  bool modifier_valid;
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
  format->modifier_width = 0;
  format->modifier_valid = true;

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
  if (*p == 'w') {
    format->modifier = kModWidth;
    p++;
    if (*p == 'f') {
      format->modifier = kModWidthFast;
      p++;
    }
    if (*p < '1' || *p > '9') {
      format->modifier_valid = false;
    } else {
      while (isdigit(*p)) {
        if (format->modifier_width <= 64) {
          format->modifier_width =
              format->modifier_width * 10 + *p - '0';
          if (format->modifier_width > 64) {
            format->modifier_width = 65;
          }
        }
        p++;
      }
      int width = format->modifier_width;
      format->modifier_valid =
          width == 8 || width == 16 || width == 32 || width == 64;
    }
    return p;
  }
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

STATIC char* ConvertBinary(unsigned long long v, char* buf, int buflen) {
  char* p = &buf[buflen];
  if (v == 0) {
    *--p = '0';
    return p;
  }
  while (v != 0) {
    char ch = (v & 1) + '0';
    *--p = ch;
    v >>= 1;
  }
  return p;
}

STATIC char* ConvertDecimalLongLong(unsigned long long v, char* buf,
                                    int buflen) {
  char* p = &buf[buflen - 1];
  if (v == 0) {
    *p = '0';
    return p;
  }
  while (v != 0) {
    // Use unsigned division/modulo directly.  lldiv() operates on *signed*
    // long long, so for values with the high bit set (e.g. ULLONG_MAX) it would
    // treat the operand as negative and produce a negative remainder, emitting
    // the digit '0' + (-1) == '/' instead of the correct decimal digits.
    char ch = (char)(v % 10) + '0';
    *p-- = ch;
    v = v / 10;
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

STATIC int Prepend(Writer writer, void* data, ConversionFormat* fmt,
                   bool negative) {
  if (negative) {
    return writer("-", 1, data);
  }
  if (fmt->prepend_sign) {
    return writer("+", 1, data);
  }
  if (fmt->prepend_space) {
    return writer(" ", 1, data);
  }
  return 0;
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
    int num_chars = Prepend(writer, data, fmt, negative);
    num_chars += PadToPrecision(writer, data, fmt, len, use_precision);
    return num_chars + writer(s, len, data);
  }
  
  // There is a field width, pad as necessary.
  int num_chars = 0;
  int padding = fmt->field_width - (int)len;
  if (fmt->left_justify) {
    // Left justify.  Pad to the right with spaces.
    if (AnyPrependNeeded(fmt, negative)) {
      num_chars += Prepend(writer, data, fmt, negative);
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
        num_chars += Prepend(writer, data, fmt, negative);
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
      num_chars += Prepend(writer, data, fmt, negative);
    }
    num_chars +=
        PadToPrecision(writer, data, fmt, len, use_precision);
    num_chars += writer(s, len, data);
  }
  return num_chars;
}

STATIC int WriteBinaryFormatted(Writer writer, void* data,
                                ConversionFormat* fmt,
                                unsigned long long value, bool upper) {
  char buffer[sizeof(unsigned long long) * 8];
  char* digits = ConvertBinary(value, buffer, sizeof(buffer));
  int digits_length = (int)(buffer + sizeof(buffer) - digits);
  if (value == 0 && fmt->precision == 0) {
    digits_length = 0;
  }
  const char* prefix =
      fmt->alternate_form && value != 0 ? (upper ? "0B" : "0b") : NULL;
  int prefix_length = prefix == NULL ? 0 : 2;
  int precision_zeroes = 0;
  if (fmt->precision != kWidthDefault &&
      fmt->precision > digits_length) {
    precision_zeroes = fmt->precision - digits_length;
  }
  int content_length = prefix_length + precision_zeroes + digits_length;
  int width_padding = fmt->field_width == kWidthDefault
                          ? 0
                          : fmt->field_width - content_length;
  if (width_padding < 0) {
    width_padding = 0;
  }
  bool width_zeroes = fmt->fill_zero && !fmt->left_justify &&
                      fmt->precision == kWidthDefault;
  int count = 0;
  if (!fmt->left_justify && !width_zeroes) {
    count += Pad(writer, data, width_padding, false);
  }
  if (prefix != NULL) {
    count += writer(prefix, 2, data);
  }
  if (width_zeroes) {
    count += Pad(writer, data, width_padding, true);
  }
  count += Pad(writer, data, precision_zeroes, true);
  if (digits_length != 0) {
    count += writer(digits, (size_t)digits_length, data);
  }
  if (fmt->left_justify) {
    count += Pad(writer, data, width_padding, false);
  }
  return count;
}

STATIC void RemoveFormatting(ConversionFormat* fmt) {
  fmt->fill_zero = false;
  fmt->prepend_sign = false;
  fmt->prepend_space = false;
}

STATIC bool IsWidthModifier(const ConversionFormat* fmt);

// Write the value of count into the address specified by p (for %n)
STATIC void WriteCount(ConversionFormat* fmt, int count, void* p) {
  if (IsWidthModifier(fmt)) {
    switch (fmt->modifier_width) {
      case 8:
        *(int8_t*)p = (int8_t)count;
        break;
      case 16:
        *(int16_t*)p = (int16_t)count;
        break;
      case 32:
        *(int32_t*)p = (int32_t)count;
        break;
      default:
        *(int64_t*)p = (int64_t)count;
        break;
    }
    return;
  }
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

STATIC bool IsWidthModifier(const ConversionFormat* fmt) {
  return fmt->modifier == kModWidth || fmt->modifier == kModWidthFast;
}

STATIC unsigned long long MaskToWidth(unsigned long long value, int width) {
  if (width >= 64) {
    return value;
  }
  return value & (((unsigned long long)1 << width) - 1);
}

STATIC long long SignExtendWidth(unsigned long long value, int width) {
  value = MaskToWidth(value, width);
  if (width < 64 &&
      (value & ((unsigned long long)1 << (width - 1))) != 0) {
    value |= ~(((unsigned long long)1 << width) - 1);
  }
  return (long long)value;
}

STATIC unsigned long long GetWidthArgument(ConversionFormat* fmt, va_list* ap,
                                           bool is_unsigned) {
  switch (fmt->modifier_width) {
    case 8:
      return (unsigned long long)va_arg(*ap, int);
    case 16:
#if defined(__6502__)
      return is_unsigned ? (unsigned long long)va_arg(*ap, unsigned int)
                         : (unsigned long long)va_arg(*ap, int);
#else
      return (unsigned long long)va_arg(*ap, int);
#endif
    case 32:
#if defined(__6502__)
      return is_unsigned ? (unsigned long long)va_arg(*ap, unsigned long)
                         : (unsigned long long)va_arg(*ap, long);
#else
      return is_unsigned ? (unsigned long long)va_arg(*ap, unsigned int)
                         : (unsigned long long)va_arg(*ap, int);
#endif
    default:
#if defined(__LP64__)
      return is_unsigned ? (unsigned long long)va_arg(*ap, unsigned long)
                         : (unsigned long long)va_arg(*ap, long);
#else
      return is_unsigned ? va_arg(*ap, unsigned long long)
                         : (unsigned long long)va_arg(*ap, long long);
#endif
  }
}

STATIC void GetNextArgument(char cmd, ConversionFormat* fmt, va_list* ap,
                            unsigned long long* value_ll, const char** value_s,
                            char* value_c, double* value_f, void** value_p,
                            bool* is_unsigned, bool* negative) {
  switch (cmd) {
    case 'd':
    case 'u':
    case 'i':
    case 'x':
    case 'X':
    case 'o':
    case 'b':
    case 'B':
      *is_unsigned = cmd == 'u' || cmd == 'x' || cmd == 'X' ||
                     cmd == 'o' || cmd == 'b' || cmd == 'B';
      if (IsWidthModifier(fmt)) {
        unsigned long long raw = GetWidthArgument(fmt, ap, *is_unsigned);
        long long signed_value = SignExtendWidth(raw, fmt->modifier_width);
        if (!*is_unsigned && signed_value < 0) {
          *negative = true;
          *value_ll =
              (unsigned long long)(-(signed_value + 1)) + 1;
        } else {
          *value_ll = *is_unsigned
                          ? MaskToWidth(raw, fmt->modifier_width)
                          : (unsigned long long)signed_value;
        }
        break;
      }
      switch (fmt->modifier) {
        case kModLong:
          *value_ll = *is_unsigned ? (long long)va_arg(*ap, unsigned long)
                                   : (long long)va_arg(*ap, long);
          break;
        case kModLongLong:
          *value_ll =
              *is_unsigned ? (long long)va_arg(*ap, unsigned long long)
                           : va_arg(*ap, long long);
          break;
        case kModChar:
          *value_ll = *is_unsigned ? (long long)va_arg(*ap, unsigned int)
                                   : (long long)va_arg(*ap, int);
          break;
        case kModShort:
          *value_ll = *is_unsigned ? (long long)va_arg(*ap, unsigned int)
                                   : (long long)va_arg(*ap, int);
          break;
        case kModSize_t:
          *value_ll = (long long)va_arg(*ap, size_t);
          *is_unsigned = true;
          break;
        case kModIntMax:
          *value_ll = *is_unsigned ? (long long)va_arg(*ap, uintmax_t)
                                   : (long long)va_arg(*ap, intmax_t);
          break;
        case kModPtrdiff_t:
          *value_ll = (long long)va_arg(*ap, ptrdiff_t);
          break;
        case kModLongDouble:
          *value_f = (int)va_arg(*ap, long double);
          break;
       default:
          *value_ll = *is_unsigned ? (long long)va_arg(*ap, unsigned int)
                                   : (long long)va_arg(*ap, int);
          break;
      }
      long long signed_value = (long long)*value_ll;
      if (!*is_unsigned && signed_value < 0) {
        *negative = true;
        *value_ll = (unsigned long long)(-(signed_value + 1)) + 1;
      }
      break;
    case 'p':
    case 'n':
      *value_p = va_arg(*ap, void*);
      break;
    case 'f':
    case 'g':
    case 'e':
      if (fmt->modifier == kModLongDouble) {
        *value_f = (double)va_arg(*ap, long double);
      } else {
        *value_f = va_arg(*ap, double);
      }
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
  unsigned long long value_ll;
  const char* value_s;
  char value_c;
  double value_f;
  void* value_p;

  while (*p != '\0') {
    if (*p == '%') {
      p++;
      ConversionFormat fmt = {0};
      p = CollectFormat(p, &fmt);
      if (!fmt.modifier_valid ||
          (IsWidthModifier(&fmt) &&
           strchr("diouxXbBn", *p) == NULL)) {
        return -1;
      }
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
        case 'b':
        case 'B': {
          bool upper = *p == 'B';
          p++;
          count += WriteBinaryFormatted(writer, data, &fmt,
                                        (unsigned long long)value_ll, upper);
          break;
        }
        case 'p':
          p++;
          v = ConvertHexPointer(value_p, buf, sizeof(buf));
          count += WriteFormatted(writer, data, &fmt, v, end - v, false, true);
          break;
        // TODO: support %a
#ifndef PRINTF_DISABLE_FLOAT
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
#endif
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
  size_t result = len;
  if (str->len >= 0) {
    size_t available = str->len > 0 ? (size_t)str->len - 1 : 0;
    if (len > available) {
      len = available;
    }
  }
  if (len != 0) {
    memcpy(str->p, s, len);
    str->p += len;
  }
  str->len -= len;
  return (int)result;
}

#ifndef PRINTF_SPECIALIZED_LONG

#if !defined(__6502__) && !defined(__risc_v__) && !defined(__aarch64__) && \
    !defined(__arm__) && !defined(__x86_64__) && !defined(__p_code__)
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

int __printf_fp(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Printf(FILEWriter, stdout, format, ap);
  va_end(ap);
  return v;
}

int __fprintf_fp(FILE* fp, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Printf(FILEWriter, fp, format, ap);
  va_end(ap);
  return v;
}

int sprintf(char* s, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, -1};
  int v = Printf(StringWriter, &data, format, ap);
  *data.p = '\0';
  va_end(ap);
  return v;
}

int __sprintf_fp(char* s, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, -1};
  int v = Printf(StringWriter, &data, format, ap);
  *data.p = '\0';
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
  if (len != 0) {
    *data.p = '\0';
  }
  va_end(ap);
  return v;
}

int __snprintf_fp(char* s, size_t len, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, len};
  int v = Printf(StringWriter, &data, format, ap);
  if (len != 0) {
    *data.p = '\0';
  }
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

#else

int __printf_long(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Printf(FILEWriter, stdout, format, ap);
  va_end(ap);
  return v;
}

int __fprintf_long(FILE* fp, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Printf(FILEWriter, fp, format, ap);
  va_end(ap);
  return v;
}

int __sprintf_long(char* s, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, -1};
  int v = Printf(StringWriter, &data, format, ap);
  *data.p = '\0';
  va_end(ap);
  return v;
}

int __snprintf_long(char* s, size_t len, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s, len};
  int v = Printf(StringWriter, &data, format, ap);
  if (len != 0) {
    *data.p = '\0';
  }
  va_end(ap);
  return v;
}

#endif

#endif
