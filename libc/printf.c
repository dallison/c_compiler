//
//  printf.c
//  c_compiler
//
//  Created by David Allison on 5/5/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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
  char fill_char;
  bool prepend_sign;
  bool prepend_space;
  bool alternate_form;
  int fw_argnum;
  int p_argnum;
  Modifier modifier;
  int next_arg_value[2];    // [0]: width, [1]: precision
} ConversionFormat;

 const char* CollectFormat(const char* p, ConversionFormat* format) {
  format->field_width = kWidthDefault;
  format->precision = kWidthDefault;
  format->fill_char = ' ';
  
  switch (*p) {
    case '-':
      p++;
      format->left_justify = true;
      break;
    case '0':
      if (!format->left_justify) {
        format->fill_char = '0';
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
  }
  if (*p == '*') {
    p++;
    format->field_width = kWidthNextArg;
    // TODO: *m$ to specify arg number.
  } else {
    int n = 0;
    while (isdigit(*p)) {
      n = n * 10 + *p++ - '0';
    }
    if (n > 0) {
      format->field_width = n;
    }
  }
  if (*p == '.') {
    p++;
    if (*p == '*') {
      p++;
      format->precision = kWidthNextArg;
      // TODO: *m$ to specify arg number.
    } else {
      int n = 0;
      while (isdigit(*p)) {
        n = n * 10 + *p++ - '0';
      }
      if (n > 0) {
        format->precision = n;
      }
    }
  }
  return p;
}

// Index is the start of the converted field in the buffer which is created
// backwards, right justified.
// This is used if a field width has been specified.
// Returns index into buffer
 int FillField(char* buf, int buflen, int index,
                      ConversionFormat* fmt) {
 
  int precision = fmt->precision;
  if (fmt->precision == kWidthNextArg) {
    precision = fmt->next_arg_value[1];
  }
  
  int len = buflen - index;     // Existing length.
   if (precision != kWidthDefault) {
     if (len < fmt->precision) {
       while (len < precision) {
         buf[--index] = '0';
         len++;
       }
     }
  }
   
  if (fmt->field_width == kWidthDefault) {
    return index;
  }
  int field_width = fmt->field_width;
  if (field_width == kWidthNextArg) {
     field_width = fmt->next_arg_value[0];
  }
  
  if (fmt->left_justify) {
    // Left justify to start of buffer.
    int new_index = buflen - field_width;
    char* left = &buf[new_index];
    memmove(left, &buf[index], len);
    index = new_index + len;
    while (len < field_width) {
       buf[index++] = fmt->fill_char;
       len++;
    }
    return new_index;
  }
  while (len < field_width) {
    buf[--index] = fmt->fill_char;
    len++;
  }
   return index;
}

char* PrintDecimal(ConversionFormat* fmt, long long v, char* buf, int buflen) {
  int i = buflen - 1;
  if (v == 0) {
    buf[i] = '0';
    i = FillField(buf, buflen, i, fmt);
    return &buf[i];
  }
  bool negative = false;
  if (v < 0) {
    negative = true;
    v = -v;
  }
  while (v != 0) {
    char ch = (v % 10) + '0';
    buf[i--] = ch;
    v /= 10;
  }
  if (negative) {
    buf[i--] = '-';
  } else {
    if (fmt->prepend_sign) {
      buf[i--] = '+';
    } else if (fmt->prepend_space) {
      buf[i--] = ' ';
    }
  }
  // i is one less than the first char.
  i++;
  i = FillField(buf, buflen, i, fmt);
  return &buf[i];
}

char* PrintHex(ConversionFormat* fmt, long long v, char* buf, int buflen, bool upper) {
  int i = buflen - 1;
  if (v == 0) {
    buf[i] = '0';
    i = FillField(buf, buflen, i, fmt);
    return &buf[i];
  }
  while (v != 0) {
    int n = v & 0xf;
    char ch;
    if (n > 9) {
      ch = n - 10 + (upper ? 'A' : 'a');
    } else {
      ch = n + '0';
    }
    buf[i--] = ch;
    v >>= 4;
  }
  i++;
  i = FillField(buf, buflen, i, fmt);
  return &buf[i];
}

char* PrintString(ConversionFormat* fmt, const char* s, int len, char* buf,
                  int buflen) {
  int i = buflen - 1;     // Last pos in buffer.
  fmt->fill_char = ' ';   // Don't use fill char
  char* p = buf + i;
  s += len - 1;               // Last char in s.
  
  // Copy s to p, backwards.
  while (len-- > 0) {
    *p-- = *s--;
    i--;
  }
  // i is the index of first char in buf.
  i = FillField(buf, buflen, i, fmt);
  return &buf[i];
}

 void EnsureBuffer(char* default_buffer, char** buf, int* buflen, Width width, va_list ap) {
  if (width == kWidthDefault) {
    return;
  }
  int width_needed = *buflen;
  if (width == kWidthNextArg) {
    width_needed = va_arg(ap, int);
  } else {
    width_needed = -(width + 2);
  }
  if (width_needed > *buflen) {
    if (*buf == default_buffer) {
      *buf = malloc(width_needed);
    } else {
      *buf = realloc(*buf, width_needed);
    }
    *buflen = width_needed;
  }
}

int vfprintf(FILE* fp, const char* format, va_list ap) {
  char default_buffer[4096];
  int buflen = sizeof(default_buffer);
  char* buf = default_buffer;
  char* v;
  int len;
  const char* p = format;
  int count = 0;
  while (*p != '\0') {
    if (*p == '%') {
      p++;
      ConversionFormat fmt = {0};
      p = CollectFormat(p, &fmt);
      EnsureBuffer(default_buffer, &buf, &buflen, fmt.field_width, ap);
      EnsureBuffer(default_buffer, &buf, &buflen, fmt.precision, ap);
      char* end = buf + buflen;
      switch (*p) {
        case 'd':
        case 'i':
          p++;
          v = PrintDecimal(&fmt, va_arg(ap, int), buf, buflen);
          len = end - v;
          count += fwrite(v, 1, len, fp);
          break;
        case 'l':
          p++;
          v = PrintDecimal(&fmt, va_arg(ap, long), buf, buflen);
          len = end - v;
          count += fwrite(v, 1, len, fp);
          break;
        case 'h':
          p++;
          v = PrintDecimal(&fmt, va_arg(ap, short), buf, buflen);
          len = end - v;
          count += fwrite(v, 1, len, fp);
          break;
        case 'x':
        case 'X':
          p++;
          v = PrintHex(&fmt, va_arg(ap, int), buf, buflen, p[-1] == 'X');
          len = end - v;
          count += fwrite(v, 1, len, fp);
          break;
        case 'c': {
          p++;
          char b[1] = {va_arg(ap, char)};
          v = PrintString(&fmt, b, 1, buf, buflen);
          len = end - v;
          count += fwrite(v, 1, len, fp);
          break;
        }
        case 's': {
          p++;
          char* s = va_arg(ap, char*);
          v = PrintString(&fmt, s, strlen(s), buf, buflen);
          len = end - v;
          count += fwrite(v, 1, len, fp);
          break;
        }
        default:
          fputc(*p++, fp);
          count++;
          break;
      }
    } else {
      fputc(*p++, fp);
      count++;
    }
  }
  if (buf != default_buffer) {
    free(buf);
  }
  return count;
}

int fprintf(FILE* fp, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = vfprintf(fp, format, ap);
  va_end(ap);
  return v;
}

int printf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = vfprintf(stdout, format, ap);
  va_end(ap);
  return v;
}

static char* SafeMemcpy(char* to, char* end, char* from, size_t len) {
  if ((to + len) > end) {
    len = end - to;
  }
  memcpy(to, from, len);
  return to + len;
}

int vsnprintf(char * restrict s, size_t n,
              const char * restrict format, va_list ap) {
  char default_buffer[4096];
  int buflen = sizeof(default_buffer);
  char* buf = default_buffer;
  char* v;
  int len;
  const char* p = format;

  char* s_start = s;
  char* s_end = s + n;
  
  while (*p != '\0' && s < s_end) {
    if (*p == '%') {
      p++;
      ConversionFormat fmt;
      p = CollectFormat(p, &fmt);
      EnsureBuffer(default_buffer, &buf, &buflen, fmt.field_width, ap);
      EnsureBuffer(default_buffer, &buf, &buflen, fmt.precision, ap);
      char* end = buf + buflen;
      switch (*p) {
        case 'd':
        case 'i':
          p++;
          v = PrintDecimal(&fmt, va_arg(ap, int), buf, buflen);
          len = end - v;
          s = SafeMemcpy(s, s_end, v, len);
          break;
        case 'l':
          p++;
          v = PrintDecimal(&fmt, va_arg(ap, long), buf, sizeof(buf));
          len = end - v;
          s = SafeMemcpy(s, s_end, v, len);
          break;
        case 'h':
          p++;
          v = PrintDecimal(&fmt, va_arg(ap, short), buf, sizeof(buf));
          len = end - v;
          s = SafeMemcpy(s, s_end, v, len);
          break;
        case 'x':
        case 'X':
          p++;
          v = PrintHex(&fmt, va_arg(ap, int), buf, sizeof(buf), p[-1] == 'X');
          len = end - v;
          s = SafeMemcpy(s, s_end, v, len);
          break;
        case 'c': {
          p++;
          char b[1] = {va_arg(ap, char)};
          v = PrintString(&fmt, b, 1, buf, buflen);
          s = SafeMemcpy(s, s_end, v, len);
          break;
        }
        case 's': {
          p++;
          char* s = va_arg(ap, char*);
          v = PrintString(&fmt, s, strlen(s), buf, buflen);
          len = end - v;
          s = SafeMemcpy(s, s_end, v, len);
          break;
        }
        default:
          *s++ = *p++;
          break;
      }
    } else {
      *s++ = *p++;
    }
  }
  return s - s_start;
}


int vsprintf(char * restrict s,
             const char * restrict format, va_list ap) {
  return vsnprintf(s, 0x7fffffffffffffffLL, format, ap);
}

int snprintf(char * restrict s, size_t n,
             const char * restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = vsnprintf(s, n, format, ap);
  va_end(ap);
  return v;
}

int sprintf(char * restrict s,
            const char * restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = vsnprintf(s, 0x7fffffffffffffffLL, format, ap);
  va_end(ap);
  return v;
}
