#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int (*SmallWriter)(const char*, size_t, void*);

typedef struct {
  int width;
  int precision;
  bool left;
  bool zero;
  bool plus;
  bool space;
  bool alternate;
  bool short_value;
  bool char_value;
  bool has_width;
  bool has_precision;
  bool width_from_arg;
  bool precision_from_arg;
} SmallFormat;

typedef struct {
  char* buffer;
  size_t size;
  size_t used;
} SmallString;

static int FileWriter(const char* text, size_t length, void* data) {
  return fwrite(text, 1, length, data) == length ? (int)length : -1;
}

static int StringWriter(const char* text, size_t length, void* data) {
  SmallString* string = data;
  size_t available =
      string->size > string->used ? string->size - string->used - 1 : 0;
  size_t copy = length < available ? length : available;
  if (copy != 0) {
    memcpy(string->buffer + string->used, text, copy);
  }
  string->used += length;
  return (int)length;
}

static const char* ParseFormat(const char* p, SmallFormat* format) {
  while (*p == '-' || *p == '0' || *p == '+' || *p == ' ' || *p == '#') {
    switch (*p++) {
      case '-': format->left = true; break;
      case '0': format->zero = true; break;
      case '+': format->plus = true; break;
      case ' ': format->space = true; break;
      case '#': format->alternate = true; break;
    }
  }
  if (*p == '*') {
    format->has_width = true;
    format->width_from_arg = true;
    p++;
  } else if (isdigit((unsigned char)*p)) {
    format->width = 0;
    format->has_width = true;
    while (isdigit((unsigned char)*p)) {
      format->width = format->width * 10 + *p++ - '0';
    }
  }
  if (*p == '.') {
    p++;
    format->precision = 0;
    format->has_precision = true;
    if (*p == '*') {
      format->precision_from_arg = true;
      p++;
    } else {
      while (isdigit((unsigned char)*p)) {
        format->precision = format->precision * 10 + *p++ - '0';
      }
    }
  }
  if (*p == 'h') {
    p++;
    if (*p == 'h') {
      format->char_value = true;
      p++;
    } else {
      format->short_value = true;
    }
  }
  return p;
}

static char* ConvertDecimal(unsigned int value, char* end) {
  *--end = '\0';
  do {
    *--end = (char)('0' + value % 10);
    value /= 10;
  } while (value != 0);
  return end;
}

static char* ConvertHex(unsigned int value, bool uppercase, char* end) {
  *--end = '\0';
  do {
    unsigned int digit = value & 15;
    *--end = digit < 10 ? (char)('0' + digit)
                        : (char)((uppercase ? 'A' : 'a') + digit - 10);
    value >>= 4;
  } while (value != 0);
  return end;
}

static char* ConvertOctal(unsigned int value, char* end) {
  *--end = '\0';
  do {
    *--end = (char)('0' + (value & 7));
    value >>= 3;
  } while (value != 0);
  return end;
}

static char* ConvertPointer(uintptr_t value, char* end) {
  *--end = '\0';
  do {
    unsigned int digit = (unsigned int)(value & 15);
    *--end = digit < 10 ? (char)('0' + digit) : (char)('a' + digit - 10);
    value >>= 4;
  } while (value != 0);
  return end;
}

static int WritePadding(SmallWriter writer, void* data, int count, char ch) {
  static const char spaces[] = "        ";
  static const char zeroes[] = "00000000";
  const char* block = ch == '0' ? zeroes : spaces;
  int written = 0;
  while (count > 0) {
    int amount = count > 8 ? 8 : count;
    int result = writer(block, (size_t)amount, data);
    if (result < 0) {
      return -1;
    }
    written += result;
    count -= amount;
  }
  return written;
}

static int WriteNumber(SmallWriter writer, void* data, SmallFormat* format,
                       const char* digits, const char* prefix, bool negative) {
  int digits_length = (int)strlen(digits);
  int zeroes = 0;
  if (format->has_precision) {
    zeroes = format->precision - digits_length;
    if (zeroes < 0) {
      zeroes = 0;
    }
  }
  char sign = negative ? '-' : format->plus ? '+' : format->space ? ' ' : 0;
  int prefix_length = prefix == NULL ? 0 : (int)strlen(prefix);
  int length = digits_length + zeroes + prefix_length + (sign != 0);
  int padding = 0;
  if (format->has_width) {
    padding = format->width - length;
    if (padding < 0) {
      padding = 0;
    }
  }
  int count = 0;
#define WRITE_PART(text, size)                                      \
  do {                                                              \
    int result = writer((text), (size), data);                       \
    if (result < 0) return -1;                                       \
    count += result;                                                 \
  } while (0)
  if (!format->left && !format->zero) {
    int result = WritePadding(writer, data, padding, ' ');
    if (result < 0) return -1;
    count += result;
  }
  if (sign != 0) {
    WRITE_PART(&sign, 1);
  }
  if (prefix_length != 0) {
    WRITE_PART(prefix, (size_t)prefix_length);
  }
  if (!format->left && format->zero) {
    int result = WritePadding(writer, data, padding, '0');
    if (result < 0) return -1;
    count += result;
  }
  {
    int result = WritePadding(writer, data, zeroes, '0');
    if (result < 0) return -1;
    count += result;
  }
  WRITE_PART(digits, (size_t)digits_length);
  if (format->left) {
    int result = WritePadding(writer, data, padding, ' ');
    if (result < 0) return -1;
    count += result;
  }
#undef WRITE_PART
  return count;
}

static int WriteText(SmallWriter writer, void* data, SmallFormat* format,
                     const char* text, size_t length) {
  if (format->has_precision && (size_t)format->precision < length) {
    length = (size_t)format->precision;
  }
  int padding = 0;
  if (format->has_width) {
    padding = format->width - (int)length;
    if (padding < 0) {
      padding = 0;
    }
  }
  int count = 0;
  if (!format->left) {
    int result = WritePadding(writer, data, padding, ' ');
    if (result < 0) return -1;
    count += result;
  }
  int result = writer(text, length, data);
  if (result < 0) return -1;
  count += result;
  if (format->left) {
    result = WritePadding(writer, data, padding, ' ');
    if (result < 0) return -1;
    count += result;
  }
  return count;
}

static int NextInt(va_list* args) {
  return va_arg(*args, int);
}

static unsigned int NextUnsignedInt(va_list* args) {
  return va_arg(*args, unsigned int);
}

static void* NextPointer(va_list* args) {
  return va_arg(*args, void*);
}

static const char* NextString(va_list* args) {
  return va_arg(*args, const char*);
}

static int PrintIntOnly(SmallWriter writer, void* data, const char* format,
                        va_list args) {
  int count = 0;
  while (*format != '\0') {
    if (*format != '%') {
      int result = writer(format++, 1, data);
      if (result < 0) return -1;
      count += result;
      continue;
    }
    format++;
    if (*format == '%') {
      int result = writer("%", 1, data);
      if (result < 0) return -1;
      count += result;
      format++;
      continue;
    }
    SmallFormat spec = {0};
    format = ParseFormat(format, &spec);
    if (spec.width_from_arg) {
      spec.width = NextInt(&args);
      if (spec.width < 0) {
        spec.left = true;
        spec.width = -spec.width;
      }
    }
    if (spec.precision_from_arg) {
      spec.precision = NextInt(&args);
      if (spec.precision < 0) {
        spec.has_precision = false;
      }
    }
    char conversion = *format == '\0' ? '\0' : *format++;
    char buffer[2 + sizeof(unsigned int) * 8 + 1];
    char* digits;
    int result;
    switch (conversion) {
      case 'd':
      case 'i': {
        int signed_value = NextInt(&args);
        if (spec.char_value) signed_value = (signed char)signed_value;
        if (spec.short_value) signed_value = (short)signed_value;
        bool negative = signed_value < 0;
        unsigned int value = negative
                                 ? (unsigned int)(-(signed_value + 1)) + 1
                                 : (unsigned int)signed_value;
        digits = ConvertDecimal(value, buffer + sizeof(buffer));
        result = WriteNumber(writer, data, &spec, digits, NULL, negative);
        break;
      }
      case 'u':
      case 'x':
      case 'X':
      case 'o': {
        unsigned int value = NextUnsignedInt(&args);
        if (spec.char_value) value = (unsigned char)value;
        if (spec.short_value) value = (unsigned short)value;
        if (conversion == 'o') {
          digits = ConvertOctal(value, buffer + sizeof(buffer));
        } else if (conversion == 'u') {
          digits = ConvertDecimal(value, buffer + sizeof(buffer));
        } else {
          digits =
              ConvertHex(value, conversion == 'X', buffer + sizeof(buffer));
        }
        const char* prefix = NULL;
        if (spec.alternate && value != 0) {
          if (conversion == 'o') {
            prefix = "0";
          } else if (conversion == 'X') {
            prefix = "0X";
          } else if (conversion == 'x') {
            prefix = "0x";
          }
        }
        result = WriteNumber(writer, data, &spec, digits, prefix, false);
        break;
      }
      case 'p': {
        uintptr_t value = (uintptr_t)NextPointer(&args);
        digits = ConvertPointer(value, buffer + sizeof(buffer));
        result = WriteNumber(writer, data, &spec, digits, "0x", false);
        break;
      }
      case 'c': {
        char ch = (char)NextInt(&args);
        result = WriteText(writer, data, &spec, &ch, 1);
        break;
      }
      case 's': {
        const char* text = NextString(&args);
        if (text == NULL) text = "(null)";
        result = WriteText(writer, data, &spec, text, strlen(text));
        break;
      }
      case 'n': {
        void* destination = NextPointer(&args);
        if (spec.char_value) *(signed char*)destination = (signed char)count;
        else if (spec.short_value) *(short*)destination = (short)count;
        else *(int*)destination = count;
        result = 0;
        break;
      }
      default:
        result = writer(&conversion, 1, data);
        break;
    }
    if (result < 0) return -1;
    count += result;
  }
  return count;
}

static int FinishString(char* buffer, size_t size, SmallString* string,
                        int result) {
  if (size != 0) {
    size_t end = string->used < size ? string->used : size - 1;
    buffer[end] = '\0';
  }
  return result;
}

int __printf_int(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = PrintIntOnly(FileWriter, stdout, format, args);
  va_end(args);
  return result;
}

int __fprintf_int(FILE* stream, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = PrintIntOnly(FileWriter, stream, format, args);
  va_end(args);
  return result;
}

int __sprintf_int(char* buffer, const char* format, ...) {
  va_list args;
  va_start(args, format);
  SmallString string = {buffer, (size_t)-1, 0};
  int result = PrintIntOnly(StringWriter, &string, format, args);
  result = FinishString(buffer, (size_t)-1, &string, result);
  va_end(args);
  return result;
}

int __snprintf_int(char* buffer, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  SmallString string = {buffer, size, 0};
  int result = PrintIntOnly(StringWriter, &string, format, args);
  result = FinishString(buffer, size, &string, result);
  va_end(args);
  return result;
}
