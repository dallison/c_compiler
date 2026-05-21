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

#if 0
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
  kModPtrdiff_t
} Modifier;

typedef enum {
  kWidthDefault = -1,
  kWidthExplicit = 0,
} Width;

typedef struct {
  bool suppress;
  Width width;
  Modifier modifier;
} ConversionFormat;

typedef struct {
  char chars[32];
  bool invert;
} ScanSet;

STATIC const char* CollectFormat(const char* p, ConversionFormat* format) {
  format->suppress = false;
  format->width = kWidthDefault;
  format->modifier = kModNone;
  
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

STATIC void WriteInt(unsigned long long v, bool is_unsigned, void* ptr, ConversionFormat* fmt) {
  switch (fmt->modifier) {
  case kModNone:
  case kModLongDouble:
      if (is_unsigned) {
        *(unsigned int*)ptr = (unsigned int)v;
      } else {
        *(int*)ptr = (int)v;
      }
      break;
  case  kModChar:
      *(char*)ptr = (char)v;
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
  int max_length = fmt->width == kWidthDefault ? 1024 : (int)fmt->width;
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
    if (sign_ok && ch == '-') {
      negative = true;
      sign_ok = false;
    } else if (sign_ok && ch == '+') {
      sign_ok = false;
    } else {
      switch (base) {
        case 8:
          if (ch >= 0 && ch < 8) {
            v = v << 3 | ch - '0';
          } else {
            goto done;
          }
          break;
        case 10:
          if (isdigit(ch)) {
            v = v * 10 + ch - '0';
          } else {
            goto done;
          }
          break;
        case 16:
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
          } else {
            goto done;
          }
          break;
        // No base specified, determine prefix
        case 0:
          if (ch == '0') {
            if (!prefix_done) {
              prefix_ok = true;
            }
          } else if (prefix_ok && (ch == 'X' || ch == 'x')) {
            prefix_ok = false;
            prefix_done = true;
            base = 16;
          } else if (prefix_ok) {
            base = 8;
          } else if (isdigit(ch)) {
            v = v * 10 + ch - '0';
            base = 10;
          } else {
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
  return true;
}

STATIC bool ReadPointer(Getter get, Ungetter ungetter, void* data, void* ptr, ConversionFormat* fmt) {
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
      break;
    }
    
  }
  if (!fmt->suppress) {
    *(void**)ptr = (void*)v;
  }
  return true;
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

STATIC int Scanf(Getter get, Ungetter unget, void* data, const char* format, va_list ap) {
  const char* p = format;
  int num_items = 0;
  bool input_error = false;
  while (*p != '\0') {
    if (*p == '%') {
      // Conversion.
      p++;
      ConversionFormat fmt;
      p = CollectFormat(p, &fmt);
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
          // Number of conversions so far.
          *(int*)ptr = num_items;
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

int main() {
  const char* input = " 1234 1234567890 0x1234 (null) abcdefgh ignore notignore";
  char buf[11], buf2[16], buf3[16], buf4[16];
  int v;
  int n;
  void *p1 = NULL, *p2 = NULL;
  sscanf(input, " %d %10c %p %p %[a-d] %n %[^a-d ] %*s %s", &v, buf, &p1, &p2, buf2, &n, buf3, buf4);
  buf[10] = '\0';
  printf("%d %s %p %p %s %d %s %s\n", v, buf, p1, p2, buf2, n, buf3, buf4);
}
