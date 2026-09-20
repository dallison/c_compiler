//
//  scanf.c
//  c_compiler
//
//  Created by David Allison on 7/1/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "_fpfuncs.h"


#if 1
#define STATIC static
#else
#define STATIC
#endif

// Function to read a single char from the entity in data.
typedef int (*Getter)(void* data);

// Function to unget a character.
typedef void (*Ungetter)(char ch, void* data);

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

typedef enum {
  kWidthDefault = -1,
  kWidthExplicit = 0,
} Width;

typedef struct {
  bool suppress;
  int width;
  Modifier modifier;
  int modifier_width;
  bool modifier_valid;
} ConversionFormat;

typedef struct {
  char chars[32];
  bool invert;
} ScanSet;

STATIC const char* CollectFormat(const char* p, ConversionFormat* format) {
  format->suppress = false;
  format->width = kWidthDefault;
  format->modifier = kModNone;
  format->modifier_width = 0;
  format->modifier_valid = true;
  
  // Optional input suppression char.
  if (*p == '*') {
    format->suppress = true;
    p++;
  }
  // Optional field width.
  if (isdigit(*p)) {
    int n = 0;
    while (isdigit(*p)) {
      n = n * 10 + *p++ - '0';
    }
    format->width = n;
  }
  
  // Optional modifier.
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
    case 'l':     // l or ll
      if (p[1] == 'l') {
        format->modifier = kModLongLong;
        p++;
      } else {
        format->modifier = kModLong;
      }
      p++;
      break;
    case 'h':   // h or hh
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


STATIC void SkipInputWhiteSpace(Getter get, Ungetter unget, void* data) {
  for (;;) {
    int ch = get(data);
    if (ch == EOF) {
      break;
    }
    if (!isspace(ch)) {
      unget(ch, data);
      break;
    }
  }
}

STATIC bool IsWidthModifier(const ConversionFormat* fmt) {
  return fmt->modifier == kModWidth || fmt->modifier == kModWidthFast;
}

STATIC void WriteInt(unsigned long long v, bool is_unsigned, void* ptr, ConversionFormat* fmt) {
  if (IsWidthModifier(fmt)) {
    switch (fmt->modifier_width) {
      case 8:
        if (is_unsigned) {
          *(uint8_t*)ptr = (uint8_t)v;
        } else {
          *(int8_t*)ptr = (int8_t)v;
        }
        break;
      case 16:
        if (is_unsigned) {
          *(uint16_t*)ptr = (uint16_t)v;
        } else {
          *(int16_t*)ptr = (int16_t)v;
        }
        break;
      case 32:
        if (is_unsigned) {
          *(uint32_t*)ptr = (uint32_t)v;
        } else {
          *(int32_t*)ptr = (int32_t)v;
        }
        break;
      default:
        if (is_unsigned) {
          *(uint64_t*)ptr = (uint64_t)v;
        } else {
          *(int64_t*)ptr = (int64_t)v;
        }
        break;
    }
    return;
  }
  switch (fmt->modifier) {
  case kModNone:
  case kModLongDouble:
      if (is_unsigned) {
        *(unsigned int*)ptr = (unsigned int)v;
      } else {
        *(int*)ptr = (int)v;
      }
      break;
  case kModChar:
      if (is_unsigned) {
        *(unsigned char*)ptr = (unsigned char)v;
      } else {
        *(signed char*)ptr = (signed char)v;
      }
      break;
  case  kModShort:
    if (is_unsigned) {
      *(unsigned short*)ptr = (unsigned short)v;
    } else {
      *(short*)ptr = (short)v;
    }
      break;
  case  kModLong:
      if (is_unsigned) {
        *(unsigned long*)ptr = (unsigned long)v;
      } else {
      *(long*)ptr = (long)v;
      }
      break;
  case  kModLongLong:
      if (is_unsigned) {
        *(unsigned long long*)ptr = v;
      } else {
        *(long long*)ptr = v;
      }
      break;
  case  kModIntMax:
      *(int*)ptr = (int)v;
      break;
  case  kModSize_t:
      *(size_t*)ptr = (size_t)v;
      break;
  case  kModPtrdiff_t:
      *(ptrdiff_t*)ptr = (ptrdiff_t)v;
      break;
  }
}

// Returns false for EOF.
STATIC bool ConvertDecimal(Getter get, Ungetter unget, int base, bool is_unsigned,
                          void* data, void* ptr, ConversionFormat* fmt) {
  int length = 0;
  int digits = 0;
  int max_length = 1024;
  if (fmt->width >= 0) {
    max_length = fmt->width;
  }
  unsigned long long v = 0;
  bool sign_ok = true;
  bool negative = false;
  bool prefix_ok = false;
  bool prefix_done = false;
  while (length < max_length) {
    int ch = get(data);
    if (ch == EOF) {
      if (length > 0) {
        goto done;
      }
      return false;
    }
    length++;
    if (sign_ok && ch == '-') {
      negative = true;
      sign_ok = false;
    } else if (sign_ok && ch == '+') {
      sign_ok = false;
    } else {
      switch (base) {
        case 8:
          if (ch >= '0' && ch <= '7') {
            v = v << 3 | ch - '0';
            digits++;
          } else {
            unget(ch, data);
            length--;
            goto done;
          }
          break;
        case 10:
          if (isdigit(ch)) {
            v = v * 10 + ch - '0';
            digits++;
          } else {
            unget(ch, data);
            length--;
            goto done;
          }
          break;
        case 16:
          if (ch == '0') {
            if (!prefix_done) {
              prefix_ok = true;
            }
            digits++;
          } else if (prefix_ok && (ch == 'X' || ch == 'x')) {
            prefix_ok = false;
            prefix_done = true;
            digits = 0;
          } else if (isxdigit(ch)) {
            char uch = toupper(ch);
            if (uch >= 'A') {
              v = v << 4 | uch - 'A' + 10;
            } else {
              v = v << 4 | ch - '0';
            }
            digits++;
          } else {
            unget(ch, data);
            length--;
            goto done;
          }
          break;
        case 2:
          if (ch == '0' && !prefix_done && digits == 0) {
            prefix_ok = true;
            digits = 1;
          } else if (prefix_ok && (ch == 'B' || ch == 'b')) {
            prefix_ok = false;
            prefix_done = true;
            digits = 0;
          } else if (ch == '0' || ch == '1') {
            v = v << 1 | ch - '0';
            digits++;
          } else {
            unget(ch, data);
            length--;
            goto done;
          }
          break;
        // No base specified, determine prefix
        case 0:
          if (ch == '0') {
            if (!prefix_done) {
              prefix_ok = true;
            }
            digits++;
          } else if (prefix_ok && (ch == 'X' || ch == 'x')) {
            prefix_ok = false;
            prefix_done = true;
            base = 16;
            digits = 0;
          } else if (prefix_ok && (ch == 'B' || ch == 'b')) {
            prefix_ok = false;
            prefix_done = true;
            base = 2;
            digits = 0;
          } else if (prefix_ok) {
            base = 8;
            if (ch >= '0' && ch <= '7') {
              v = v << 3 | ch - '0';
              digits++;
            } else {
              unget(ch, data);
              length--;
              goto done;
            }
          } else if (isdigit(ch)) {
            v = v * 10 + ch - '0';
            base = 10;
            digits++;
          } else {
            unget(ch, data);
            length--;
            goto done;
          }
          break;
      }
    }
  }
done:
  if (negative) {
    v = -v;
  }
  if (!fmt->suppress) {
    WriteInt(v, is_unsigned, ptr, fmt);
  }
  return digits > 0;
}

STATIC bool ReadPointer(Getter get, Ungetter unget, void* data, void* ptr, ConversionFormat* fmt) {
  int length = 0;
  int max_length = fmt->width == kWidthDefault ? INT_MAX : (int)fmt->width;
  unsigned long long v = 0;
  bool prefix_done = false;
  bool prefix_ok = false;
  int null_index = 1;
  bool is_null = false;
  const char null_literal[] = "(null)";
  
  while (length < max_length) {
    int ch = get(data);
    if (ch == EOF) {
      if (length > 0) {
        break;
      }
      return false;
    }
    length++;
    if (ch == '(') {
      // "(null)"
      is_null = true;
      continue;
    }
    if (ch == '0') {
      if (!prefix_done) {
        prefix_ok = true;
      }
    } else if (prefix_ok && (ch == 'X' || ch == 'x')) {
      prefix_ok = false;
      prefix_done = true;
    } else if (isxdigit(ch)) {
      char uch = toupper(ch);
      if (uch > 'A') {
        v = v << 4 | uch - 'A' + 10;
      } else {
        v = v << 4 | ch - '0';
      }
    } else if (is_null) {
      if (null_index == sizeof(null_literal) - 1) {
        v = 0;
        break;
      }
      if (ch != null_literal[null_index]) {
        return false;
      }
      null_index++;
    } else {
      unget(ch, data);
      length--;
      break;
    }
    
  }
  if (!fmt->suppress) {
    *(void**)ptr = (void*)v;
  }
  return length > 0;
}

STATIC bool ReadChars(Getter get, Ungetter ungetter, void* data, void* ptr, ConversionFormat* fmt) {
  int numchars = fmt->width == kWidthDefault ? 1 : fmt->width;
  char* p = ptr;
  wchar_t* lp = ptr;
  int num_read = 0;
  for (int i = 0; i < numchars; i++) {
    int ch = get(data);
    if (ch == EOF) {
      break;
    }
    num_read++;
    if (!fmt->suppress) {
      if (fmt->modifier == kModLong) {
        // TODO: convert from multibyte.
        *lp++ = ch;
      } else {
        *p++ = ch;
      }
    }
  }
  if (fmt->suppress) {
    return num_read > 0;
  }
  if (fmt->modifier == kModLong) {
    if (lp == ptr) {
      return false;
    }
  } else {
    if (p == ptr) {
      return false;
    }
  }
  return true;
}

// Read string up until whitespace.
STATIC bool ReadString(Getter get, Ungetter unget, void* data, void* ptr, ConversionFormat* fmt) {
  int numchars = fmt->width == kWidthDefault ? INT_MAX : fmt->width;
  char* p = ptr;
  wchar_t* lp = ptr;
  int num_read = 0;
  for (int i = 0; i < numchars; i++) {
    int ch = get(data);
    if (ch == EOF) {
      break;
    }
    if (isspace(ch)) {
      unget(ch, data);
      break;
    }
    num_read++;
    if (!fmt->suppress) {
      if (fmt->modifier == kModLong) {
        // TODO: convert from multibyte.
        *lp++ = ch;
      } else {
        *p++ = ch;
      }
    }
  }
  if (fmt->suppress) {
    return num_read > 0;
  }
  if (fmt->modifier == kModLong) {
    if (lp == ptr) {
      return false;
    }
    *lp = L'\0';
  } else {
    if (p == ptr) {
      return false;
    }
    *p = '\0';
  }
 
  return true;
}

// [ has been consumed.
STATIC const char* ParseScanSet(const char* p, ScanSet* set) {
  int num_chars = 0;
  char* s = set->chars;
  if (*p == '^') {
    set->invert = true;
    p++;
  }
  while (*p != '\0') {
    if (*p == ']') {
      if (num_chars == 0) {
        // An empty scanset with with ] is ] itself.
        *s++ = ']';
        p++;
        num_chars++;
      } else {
        p++;
        break;
      }
    }
    *s++ = *p++;
    num_chars++;
  }
  *s++ = '\0';
  return p;
}

STATIC bool InScanSet(char ch, ScanSet* set) {
  char* p = set->chars;
  bool in_set = false;
  while (*p != '\0') {
    if (ch == *p) {
      // Char is in scan set directly.
      in_set = true;
      break;
    } else {
      if (p[1] == '-') {
        // Range?
        char start = *p;
        char end = p[2];
        for (char c = start; c != end + 1; c++) {
          if (c == ch) {
            in_set = true;
            break;
          }
        }
        if (in_set) {
          break;
        }
      }
    }
    p++;
  }
  if (set->invert) {
    return !in_set;
  }
  return in_set;
}

STATIC bool ReadScanSet(Getter get, Ungetter unget, void* data, void* ptr, ScanSet* set, ConversionFormat* fmt) {
  int numchars = fmt->width == kWidthDefault ? INT_MAX : fmt->width;
  char* p = ptr;
  wchar_t* lp = ptr;
  int num_read = 0;
  for (int i = 0; i < numchars; i++) {
    int ch = get(data);
    if (ch == EOF) {
      break;
    }
    if (!InScanSet(ch, set)) {
      unget(ch, data);
      break;
    }
    num_read++;
    if (!fmt->suppress) {
      if (fmt->modifier == kModLong) {
        // TODO: convert from multibyte.
        *lp++ = ch;
      } else {
        *p++ = ch;
      }
    }
  }
  if (fmt->suppress) {
    return num_read > 0;
  }
  if (fmt->modifier == kModLong) {
    if (lp == ptr) {
      return false;
    }
    *lp = L'\0';
  } else {
    if (p == ptr) {
      return false;
    }
    *p = '\0';
  }
 
  return true;
}

// See strtod.c for the algorithm used here.
STATIC bool ReadDouble (Getter get, Ungetter unget, void* data, void* ptr, ConversionFormat* fmt) {
  uint64_t fx[FIXED_SIZE_WORDS] = {0};
  uint64_t n[FIXED_SIZE_HALF] = {0};
  double v = 0;
  char text[512];
  int text_len = 0;
  
  uint8_t negative = 0;
  bool present = false;

#define SCANF_TAKE(ch)                                                 \
  do {                                                                 \
    if (text_len < (int)sizeof(text) - 1) {                            \
      text[text_len++] = (char)(ch);                                   \
    }                                                                  \
  } while (0)
  
  int ch = get(data);
  if (ch == EOF) {
    goto done;
  }
  if (ch == '-') {
    SCANF_TAKE(ch);
    ch = get(data);
    if (ch == EOF) {
      goto done;
    }
    negative = 0x80;
  }
  if (ch == '+') {
    SCANF_TAKE(ch);
    ch = get(data);
    if (ch == EOF) {
      goto done;
    }
  }
  
  // Convert integral part to binary.  This is put into the top
  // half of the fixed point number - the integral part.
  int fraction_digits = 0;
  while (isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E') {
    SCANF_TAKE(ch);
    n[0] = ch - '0';
    __MultiplyBy10Half(fx + FIXED_SIZE_HALF);
    __AddHalf(fx + FIXED_SIZE_HALF, n);
    ch = get(data);
    if (ch == EOF) {
      goto done;
    }
    present = true;
  }
  
  if (ch == '.') {
    SCANF_TAKE(ch);
    ch = get(data);
    if (ch == EOF) {
      goto done;
    }
    // We have a fractional part, continue accumulating and count the
    // number of fractional digits.
    while (isdigit(ch) && ch != 'e' && ch != 'E') {
      SCANF_TAKE(ch);
      n[0] = ch - '0';
      __MultiplyBy10Half(fx + FIXED_SIZE_HALF);
      __AddHalf(fx + FIXED_SIZE_HALF, n);
      ch = get(data);
      if (ch == EOF) {
        goto done;
      }
      present = true;
      fraction_digits++;
    }
    
    // Now shift the fractional part down to the lower half - to the right
    // of the binary point.
    for (int i = 0; i < fraction_digits; i++) {
      __DivideBy10(fx);
    }
  }
  
  // Check for exponent.
  if (ch == 'e' || ch == 'E') {
    SCANF_TAKE(ch);
    ch = get(data);
    if (ch == EOF) {
      goto done;
    }
    bool negative_exp = ch == '-';
    if (ch == '+' || ch =='-') {
      SCANF_TAKE(ch);
      ch = get(data);
      if (ch == EOF) {
        goto done;
      }
    }
    int exp = 0;
    while (isdigit(ch)) {
      SCANF_TAKE(ch);
      exp = exp * 10 + ch - '0';
      ch = get(data);
      if (ch == EOF) {
        goto done;
      }
    }
    
    // Multiply or divide by the exponent.
    if (negative_exp) {
      for (int i = 0; i < exp; i++) {
        __DivideBy10(fx);
      }
    } else {
      for (int i = 0; i < exp; i++) {
        __MultiplyBy10(fx);
      }
    }
  }
  // We will always go one char too far.
  unget(ch, data);
  
  // We now have a fixed point binary number. Float the binary point to normalize
  // the number.
  int exp = EXP_BIAS;
  if (__IsZero(fx)) {
    return 0;
  }
  
  // Normalize by making the integer part 1.
  if (__IsZeroHalf(fx + FIXED_SIZE_HALF)) {
    // Number is less than one, shift left.
    while (!__IsOneHalf(fx + FIXED_SIZE_HALF)) {
      __LShift(fx);
      exp--;
    }
  } else {
    // Number is greater than one, shift right.
    while (!__IsOneHalf(fx + FIXED_SIZE_HALF)) {
      __RShift(fx);
      exp++;
    }
  }
  
  // The mantissa is the upper bits of the fractional part.
  uint64_t mantissa = fx[FIXED_SIZE_HALF-1];
#if DOUBLE_IS_SINGLE
  // Single precision - 23 bit mantissa.
#if defined(__6502__)
  // 6502 is slow for multiple shifts.  This is in assembly language.
  v = __packIEEE754(negative, exp, mantissa);
#else
  uint32_t packed = (mantissa >> (32 - 23)) | (exp << 23) | negative << 24;
  v = *(double*)&packed;
#endif
#else
  // Double precision.
  uint64_t packed = (mantissa >> (64LL - 52LL)) |
          ((uint64_t)exp << 52LL) |
          (uint64_t)negative << 56LL;
  v = *(double*)&packed;
#endif

  if (!fmt->suppress) {
    if (fmt->modifier == kModLongDouble &&
        sizeof(long double) > sizeof(double)) {
      text[text_len] = '\0';
      // &text[0], not `text`: wasm32 currently materializes a decayed
      // char[] call argument as a null pointer.  The indexed stores
      // above write the digits correctly; only the decayed pointer is
      // wrong.
      long double ld = strtold(&text[0], NULL);
      memcpy(ptr, &ld, sizeof(ld));
    } else {
      *(double*)ptr = v;
    }
  }
  
done:
#undef SCANF_TAKE
  return present;
}

typedef struct {
  Getter get;
  Ungetter unget;
  void* data;
  int count;
} CountingInput;

STATIC int CountingGet(void* data) {
  CountingInput* input = data;
  int ch = input->get(input->data);
  if (ch != EOF) {
    input->count++;
  }
  return ch;
}

STATIC void CountingUnget(char ch, void* data) {
  CountingInput* input = data;
  input->unget(ch, input->data);
  input->count--;
}

STATIC int Scanf(Getter get, Ungetter unget, void* data, const char* format, va_list ap) {
  CountingInput input = {get, unget, data, 0};
  get = CountingGet;
  unget = CountingUnget;
  data = &input;
  const char* p = format;
  int num_items = 0;
  bool input_error = false;
  while (*p != '\0') {
    if (*p == '%') {
      // Conversion.
      p++;
      ConversionFormat fmt;
      p = CollectFormat(p, &fmt);
      if (!fmt.modifier_valid ||
          (IsWidthModifier(&fmt) &&
           strchr("diouxXbn", *p) == NULL)) {
        goto done;
      }
      void *ptr = NULL;
      if (!fmt.suppress) {
        ptr = va_arg(ap, void*);
      }
      if (*p != '[' && *p != 'c' && *p != 'n') {
        SkipInputWhiteSpace(get, unget, data);
      }
      char ch;
      switch (*p) {
        case '%':
          // %% is a %.
          ch = get(data);
          if (ch == EOF) {
            input_error = true;
            goto done;
          }
          if (ch != '%') {
            unget(ch, data);
            goto done;
          }
          break;
        case 'd':
        case 'u':
          // %d is decimal base 10.
          if (!ConvertDecimal(get, unget, 10, *p == 'u', data, ptr,
                              &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
        case 'i':
          // %d is decimal with undetermined base.
          if (!ConvertDecimal(get, unget, 0, false, data, ptr,
                             &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
        case 'o':
        case 'O':
          // Octal.
          if (!ConvertDecimal(get, unget, 8, true, data, ptr,
                             &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
         break;
        case 'x':
        case 'X':
          // Hex.
          if (!ConvertDecimal(get, unget, 16, true, data, ptr,
                             &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
         break;
        case 'b':
          // Binary, with an optional 0b or 0B prefix.
          if (!ConvertDecimal(get, unget, 2, true, data, ptr, &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
        case 's':
          if (!ReadString(get, unget, data, ptr, &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
        case 'c':
          if (!ReadChars(get, unget, data, ptr, &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
        case 'p':
          if (!ReadPointer(get, unget, data, ptr, &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
        case 'n':
          // Number of input characters consumed so far.
          WriteInt((unsigned long long)input.count, false, ptr, &fmt);
          p++;
         break;
        case '[': {
          p++;
          ScanSet set;
          p = ParseScanSet(p, &set);
          if (!ReadScanSet(get, unget, data, ptr, &set, &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          break;
        }
        case 'a':
        case 'e':
        case 'f':
        case 'g':
        case 'A':
        case 'E':
        case 'F':
        case 'G':
          if (!ReadDouble(get, unget, data, ptr, &fmt)) {
            input_error = true;
            goto done;
          }
          num_items++;
          p++;
          break;
          
       default:
          break;
      }
    } else {
      // Single char
      // TODO: multibyte
      if (isspace(*p)) {
        while (isspace(*p)) {
          // Skip all whitespace directives.
          p++;
        }
        // Read sequence of white space chars, stopping at EOF or first
        // non-whitespace.
        SkipInputWhiteSpace(get, unget, data);
      } else {
        int ch= get(data);
        if (ch == EOF) {
          input_error = true;
          break;
        }
        if (ch != *p) {
          unget(ch, data);
          goto done;
        }
        p++;
      }
    }
  }
done:
  // Returns EOF if input error occurs before any conversion.
  if (input_error && num_items == 0) {
    return EOF;
  }
  return num_items;
}

STATIC int FILEGet(void* data) {
  FILE* fp = data;
  return fgetc(fp);
}

STATIC void FILEUnget(char ch, void* data) {
  FILE* fp = data;
  ungetc((int)ch, fp);
}

typedef struct {
  const char* p;            // Current place to read from.
} StringData;

// Reader function to read from a string pointer.
STATIC int StringGet(void* data) {
  StringData* str = data;
  return *str->p++;
}

STATIC void StringUnget(char ch, void* data) {
  StringData* str = data;
  str->p--;
}

int scanf(const char * restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Scanf(FILEGet, FILEUnget, stdin, format, ap);
  va_end(ap);
  return v;
}

int vscanf(const char * restrict format, va_list arg) {
  return Scanf(FILEGet, FILEUnget, stdin, format, arg);
}

int fscanf(FILE * restrict stream,
           const char * restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  int v = Scanf(FILEGet, FILEUnget, stream, format, ap);
  va_end(ap);
  return v;
}

int vfscanf(FILE * restrict stream,
            const char * restrict format, va_list arg) {
  return Scanf(FILEGet, FILEUnget, stream, format, arg);
}

int sscanf(const char * restrict s,
           const char * restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  StringData data = {s};
  int v = Scanf(StringGet, StringUnget, &data, format, ap);
  va_end(ap);
  return v;
}

int vsscanf(const char * restrict s,
            const char * restrict format, va_list arg) {
  StringData data = {s};
  return Scanf(StringGet, StringUnget, &data, format, arg);
}
