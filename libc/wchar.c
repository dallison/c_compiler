#include <wchar.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static mbstate_t internal_input_state;
static mbstate_t internal_output_state;
static mbstate_t legacy_input_state;
static mbstate_t legacy_output_state;

static void reset_state(mbstate_t* state) {
  state->__value = 0;
  state->__pending = 0;
  state->__count = 0;
  state->__expected = 0;
  state->__has_pending = 0;
}

int mbsinit(const mbstate_t* state) {
  return state == NULL ||
         (state->__count == 0 && state->__has_pending == 0);
}

static size_t decode_utf8(wchar_t* output, const char* string, size_t count,
                          mbstate_t* state) {
  size_t consumed = 0;
  unsigned int value;
  unsigned int minimum;

  if (count == 0) return (size_t)-2;
  if (state->__count == 0) {
    unsigned char first = (unsigned char)string[consumed++];
    if (first < 0x80) {
      if (output != NULL) *output = (wchar_t)first;
      return first == 0 ? 0 : 1;
    }
    if (first >= 0xc2 && first <= 0xdf) {
      state->__value = first & 0x1f;
      state->__expected = 2;
    } else if (first >= 0xe0 && first <= 0xef) {
      state->__value = first & 0x0f;
      state->__expected = 3;
    } else if (first >= 0xf0 && first <= 0xf4) {
      state->__value = first & 0x07;
      state->__expected = 4;
    } else {
      errno = EILSEQ;
      reset_state(state);
      return (size_t)-1;
    }
    state->__count = 1;
  }

  while (state->__count < state->__expected && consumed < count) {
    unsigned char next = (unsigned char)string[consumed];
    if ((next & 0xc0) != 0x80) {
      errno = EILSEQ;
      reset_state(state);
      return (size_t)-1;
    }
    state->__value = (state->__value << 6) | (next & 0x3f);
    state->__count++;
    consumed++;
  }
  if (state->__count < state->__expected) return (size_t)-2;

  value = state->__value;
  minimum = state->__expected == 2 ? 0x80
            : state->__expected == 3 ? 0x800
                                     : 0x10000;
  if (value < minimum || value > 0x10ffff ||
      (value >= 0xd800 && value <= 0xdfff) ||
      value > (unsigned int)WCHAR_MAX) {
    errno = EILSEQ;
    reset_state(state);
    return (size_t)-1;
  }
  reset_state(state);
  if (output != NULL) *output = (wchar_t)value;
  return consumed;
}

size_t mbrtowc(wchar_t* restrict output, const char* restrict string,
               size_t count, mbstate_t* restrict state) {
  if (state == NULL) state = &internal_input_state;
  if (string == NULL) {
    reset_state(state);
    return 0;
  }
  return decode_utf8(output, string, count, state);
}

size_t mbrlen(const char* restrict string, size_t count,
              mbstate_t* restrict state) {
  return mbrtowc(NULL, string, count, state);
}

size_t wcrtomb(char* restrict output, wchar_t wide,
               mbstate_t* restrict state) {
  unsigned int value = (unsigned int)wide;
  if (state == NULL) state = &internal_output_state;
  if (output == NULL) {
    reset_state(state);
    return 1;
  }
  reset_state(state);
  if (wide < 0 || value > 0x10ffff ||
      (value >= 0xd800 && value <= 0xdfff)) {
    errno = EILSEQ;
    return (size_t)-1;
  }
  if (value < 0x80) {
    output[0] = (char)value;
    return 1;
  }
  if (value < 0x800) {
    output[0] = (char)(0xc0 | (value >> 6));
    output[1] = (char)(0x80 | (value & 0x3f));
    return 2;
  }
  if (value < 0x10000) {
    output[0] = (char)(0xe0 | (value >> 12));
    output[1] = (char)(0x80 | ((value >> 6) & 0x3f));
    output[2] = (char)(0x80 | (value & 0x3f));
    return 3;
  }
  output[0] = (char)(0xf0 | (value >> 18));
  output[1] = (char)(0x80 | ((value >> 12) & 0x3f));
  output[2] = (char)(0x80 | ((value >> 6) & 0x3f));
  output[3] = (char)(0x80 | (value & 0x3f));
  return 4;
}

size_t mbsrtowcs(wchar_t* restrict output, const char** restrict source,
                 size_t count, mbstate_t* restrict state) {
  const char* current = *source;
  size_t written = 0;
  mbstate_t local;
  if (state == NULL) {
    reset_state(&local);
    state = &local;
  }
  while (*current != '\0') {
    wchar_t value;
    size_t available = strlen(current) + 1;
    size_t consumed = mbrtowc(&value, current, available, state);
    if (consumed == (size_t)-1 || consumed == (size_t)-2) {
      if (output != NULL) *source = current;
      return (size_t)-1;
    }
    if (output != NULL) {
      if (written == count) {
        *source = current;
        return written;
      }
      output[written] = value;
    }
    written++;
    current += consumed;
  }
  if (output != NULL) {
    if (written < count) output[written] = L'\0';
    *source = NULL;
  }
  return written;
}

size_t wcsrtombs(char* restrict output, const wchar_t** restrict source,
                 size_t count, mbstate_t* restrict state) {
  const wchar_t* current = *source;
  size_t written = 0;
  char bytes[4];
  while (*current != L'\0') {
    size_t length = wcrtomb(bytes, *current, state);
    size_t i;
    if (length == (size_t)-1) {
      if (output != NULL) *source = current;
      return (size_t)-1;
    }
    if (output != NULL && written + length > count) {
      *source = current;
      return written;
    }
    if (output != NULL) {
      for (i = 0; i < length; i++) output[written + i] = bytes[i];
    }
    written += length;
    current++;
  }
  if (output != NULL) {
    if (written < count) output[written] = '\0';
    *source = NULL;
  }
  return written;
}

int mblen(const char* string, size_t count) {
  size_t result;
  if (string == NULL) {
    reset_state(&legacy_input_state);
    return 0;
  }
  result = mbrlen(string, count, &legacy_input_state);
  return result == (size_t)-1 || result == (size_t)-2 ? -1 : (int)result;
}

int mbtowc(wchar_t* restrict output, const char* restrict string,
           size_t count) {
  size_t result;
  if (string == NULL) {
    reset_state(&legacy_input_state);
    return 0;
  }
  result = mbrtowc(output, string, count, &legacy_input_state);
  return result == (size_t)-1 || result == (size_t)-2 ? -1 : (int)result;
}

int wctomb(char* output, wchar_t value) {
  size_t result;
  if (output == NULL) {
    reset_state(&legacy_output_state);
    return 0;
  }
  result = wcrtomb(output, value, &legacy_output_state);
  return result == (size_t)-1 ? -1 : (int)result;
}

size_t mbstowcs(wchar_t* restrict output, const char* restrict string,
                size_t count) {
  const char* source = string;
  mbstate_t state;
  reset_state(&state);
  return mbsrtowcs(output, &source, count, &state);
}

size_t wcstombs(char* restrict output, const wchar_t* restrict string,
                size_t count) {
  const wchar_t* source = string;
  mbstate_t state;
  reset_state(&state);
  return wcsrtombs(output, &source, count, &state);
}

size_t wcslen(const wchar_t* string) {
  const wchar_t* end = string;
  while (*end != L'\0') end++;
  return (size_t)(end - string);
}

wchar_t* wcscpy(wchar_t* restrict destination,
                 const wchar_t* restrict source) {
  wchar_t* result = destination;
  while ((*destination++ = *source++) != L'\0') {}
  return result;
}

wchar_t* wcsncpy(wchar_t* restrict destination,
                  const wchar_t* restrict source, size_t count) {
  wchar_t* result = destination;
  while (count != 0 && *source != L'\0') {
    *destination++ = *source++;
    count--;
  }
  while (count-- != 0) *destination++ = L'\0';
  return result;
}

wchar_t* wcscat(wchar_t* restrict destination,
                 const wchar_t* restrict source) {
  wcscpy(destination + wcslen(destination), source);
  return destination;
}

wchar_t* wcsncat(wchar_t* restrict destination,
                  const wchar_t* restrict source, size_t count) {
  wchar_t* output = destination + wcslen(destination);
  while (count-- != 0 && *source != L'\0') *output++ = *source++;
  *output = L'\0';
  return destination;
}

int wcscmp(const wchar_t* left, const wchar_t* right) {
  while (*left == *right && *left != L'\0') {
    left++;
    right++;
  }
  return *left < *right ? -1 : *left != *right;
}

int wcsncmp(const wchar_t* left, const wchar_t* right, size_t count) {
  while (count != 0 && *left == *right && *left != L'\0') {
    left++;
    right++;
    count--;
  }
  if (count == 0) return 0;
  return *left < *right ? -1 : *left != *right;
}

int wmemcmp(const wchar_t* left, const wchar_t* right, size_t count) {
  size_t i;
  for (i = 0; i < count; i++) {
    if (left[i] != right[i]) return left[i] < right[i] ? -1 : 1;
  }
  return 0;
}

wchar_t* wmemcpy(wchar_t* restrict destination,
                  const wchar_t* restrict source, size_t count) {
  size_t i;
  for (i = 0; i < count; i++) destination[i] = source[i];
  return destination;
}

wchar_t* wmemmove(wchar_t* destination, const wchar_t* source, size_t count) {
  if (destination < source) {
    return wmemcpy(destination, source, count);
  }
  while (count != 0) {
    count--;
    destination[count] = source[count];
  }
  return destination;
}

wchar_t* wmemset(wchar_t* destination, wchar_t value, size_t count) {
  size_t i;
  for (i = 0; i < count; i++) destination[i] = value;
  return destination;
}

wchar_t* wmemchr(const wchar_t* memory, wchar_t value, size_t count) {
  size_t i;
  for (i = 0; i < count; i++) {
    if (memory[i] == value) return (wchar_t*)(memory + i);
  }
  return NULL;
}

wchar_t* wcschr(const wchar_t* string, wchar_t value) {
  do {
    if (*string == value) return (wchar_t*)string;
  } while (*string++ != L'\0');
  return NULL;
}

wchar_t* wcsrchr(const wchar_t* string, wchar_t value) {
  const wchar_t* result = NULL;
  do {
    if (*string == value) result = string;
  } while (*string++ != L'\0');
  return (wchar_t*)result;
}

size_t wcsspn(const wchar_t* string, const wchar_t* accept) {
  size_t count = 0;
  while (string[count] != L'\0' &&
         wcschr(accept, string[count]) != NULL) count++;
  return count;
}

size_t wcscspn(const wchar_t* string, const wchar_t* reject) {
  size_t count = 0;
  while (string[count] != L'\0' &&
         wcschr(reject, string[count]) == NULL) count++;
  return count;
}

wchar_t* wcspbrk(const wchar_t* string, const wchar_t* accept) {
  while (*string != L'\0') {
    if (wcschr(accept, *string) != NULL) return (wchar_t*)string;
    string++;
  }
  return NULL;
}

wchar_t* wcsstr(const wchar_t* string, const wchar_t* substring) {
  size_t length = wcslen(substring);
  if (length == 0) return (wchar_t*)string;
  while (*string != L'\0') {
    if (wcsncmp(string, substring, length) == 0) return (wchar_t*)string;
    string++;
  }
  return NULL;
}

wchar_t* wcstok(wchar_t* restrict string, const wchar_t* restrict delimiters,
                wchar_t** restrict state) {
  wchar_t* token;
  if (string == NULL) string = *state;
  string += wcsspn(string, delimiters);
  if (*string == L'\0') {
    *state = string;
    return NULL;
  }
  token = string;
  string += wcscspn(string, delimiters);
  if (*string != L'\0') *string++ = L'\0';
  *state = string;
  return token;
}

int wcscoll(const wchar_t* left, const wchar_t* right) {
  return wcscmp(left, right);
}

size_t wcsxfrm(wchar_t* restrict destination,
               const wchar_t* restrict source, size_t count) {
  size_t length = wcslen(source);
  if (destination != NULL && count != 0) {
    size_t copied = length < count - 1 ? length : count - 1;
    wmemcpy(destination, source, copied);
    destination[copied] = L'\0';
  }
  return length;
}

wint_t fputwc(wchar_t value, FILE* stream) {
  char bytes[4];
  size_t length = wcrtomb(bytes, value, NULL);
  size_t i;
  if (fwide(stream, 1) < 0 || length == (size_t)-1) return WEOF;
  for (i = 0; i < length; i++) {
    if (fputc((unsigned char)bytes[i], stream) == EOF) return WEOF;
  }
  return (wint_t)value;
}

wint_t putwc(wchar_t value, FILE* stream) { return fputwc(value, stream); }
wint_t putwchar(wchar_t value) { return fputwc(value, stdout); }

int fputws(const wchar_t* restrict string, FILE* restrict stream) {
  while (*string != L'\0') {
    if (fputwc(*string++, stream) == WEOF) return -1;
  }
  return 0;
}

wint_t fgetwc(FILE* stream) {
  mbstate_t state;
  char byte;
  wchar_t value;
  size_t result;
  if (fwide(stream, 1) < 0) return WEOF;
  reset_state(&state);
  do {
    int input = fgetc(stream);
    if (input == EOF) return WEOF;
    byte = (char)input;
    result = mbrtowc(&value, &byte, 1, &state);
  } while (result == (size_t)-2);
  return result == (size_t)-1 ? WEOF : (wint_t)value;
}

wint_t getwc(FILE* stream) { return fgetwc(stream); }
wint_t getwchar(void) { return fgetwc(stdin); }

wchar_t* fgetws(wchar_t* restrict buffer, int count, FILE* restrict stream) {
  int index = 0;
  if (count <= 0) return NULL;
  while (index + 1 < count) {
    wint_t value = fgetwc(stream);
    if (value == WEOF) break;
    buffer[index++] = (wchar_t)value;
    if (value == L'\n') break;
  }
  if (index == 0) return NULL;
  buffer[index] = L'\0';
  return buffer;
}

wint_t ungetwc(wint_t value, FILE* stream) {
  char bytes[4];
  size_t length;
  if (value == WEOF || fwide(stream, 1) < 0) return WEOF;
  length = wcrtomb(bytes, (wchar_t)value, NULL);
  if (length == (size_t)-1) return WEOF;
  while (length != 0) {
    length--;
    if (ungetc((unsigned char)bytes[length], stream) == EOF) return WEOF;
  }
  return value;
}

int fwide(FILE* stream, int mode) {
  if (stream->orientation == 0 && mode != 0) {
    stream->orientation = mode > 0 ? 1 : -1;
  }
  return stream->orientation;
}

static char* wide_format_to_multibyte(const wchar_t* wide) {
  const wchar_t* source = wide;
  mbstate_t state;
  size_t length;
  char* result;
  reset_state(&state);
  length = wcsrtombs(NULL, &source, 0, &state);
  if (length == (size_t)-1) return NULL;
  result = (char*)malloc(length + 1);
  if (result == NULL) return NULL;
  source = wide;
  reset_state(&state);
  if (wcsrtombs(result, &source, length + 1, &state) == (size_t)-1) {
    free(result);
    return NULL;
  }
  return result;
}

int vfwprintf(FILE* restrict stream, const wchar_t* restrict format,
              va_list arguments) {
  char* narrow_format = wide_format_to_multibyte(format);
  int result;
  if (narrow_format == NULL || fwide(stream, 1) < 0) {
    free(narrow_format);
    return -1;
  }
  result = vfprintf(stream, narrow_format, arguments);
  free(narrow_format);
  return result;
}

int fwprintf(FILE* restrict stream, const wchar_t* restrict format, ...) {
  va_list arguments;
  int result;
  va_start(arguments, format);
  result = vfwprintf(stream, format, arguments);
  va_end(arguments);
  return result;
}

int vfwscanf(FILE* restrict stream, const wchar_t* restrict format,
             va_list arguments) {
  char* narrow_format = wide_format_to_multibyte(format);
  int result;
  if (narrow_format == NULL || fwide(stream, 1) < 0) {
    free(narrow_format);
    return EOF;
  }
  result = vfscanf(stream, narrow_format, arguments);
  free(narrow_format);
  return result;
}

int fwscanf(FILE* restrict stream, const wchar_t* restrict format, ...) {
  va_list arguments;
  int result;
  va_start(arguments, format);
  result = vfwscanf(stream, format, arguments);
  va_end(arguments);
  return result;
}

int vswprintf(wchar_t* restrict buffer, size_t size,
              const wchar_t* restrict format, va_list arguments) {
  char* narrow_format = wide_format_to_multibyte(format);
  char* narrow_output;
  const char* source;
  mbstate_t state;
  size_t wide_length;
  int result;
  size_t capacity;
  if (narrow_format == NULL || size == 0) {
    free(narrow_format);
    return -1;
  }
  capacity = size * 4 + 1;
  narrow_output = (char*)malloc(capacity);
  if (narrow_output == NULL) {
    free(narrow_format);
    return -1;
  }
  result = vsnprintf(narrow_output, capacity, narrow_format, arguments);
  free(narrow_format);
  if (result < 0 || (size_t)result >= capacity) {
    free(narrow_output);
    return -1;
  }
  source = narrow_output;
  reset_state(&state);
  wide_length = mbsrtowcs(buffer, &source, size, &state);
  free(narrow_output);
  if (wide_length == (size_t)-1 || source != NULL) return -1;
  return (int)wide_length;
}

int swprintf(wchar_t* restrict buffer, size_t size,
             const wchar_t* restrict format, ...) {
  va_list arguments;
  int result;
  va_start(arguments, format);
  result = vswprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}

int vswscanf(const wchar_t* restrict buffer,
             const wchar_t* restrict format, va_list arguments) {
  char* narrow_buffer = wide_format_to_multibyte(buffer);
  char* narrow_format = wide_format_to_multibyte(format);
  int result;
  if (narrow_buffer == NULL || narrow_format == NULL) {
    free(narrow_buffer);
    free(narrow_format);
    return EOF;
  }
  result = vsscanf(narrow_buffer, narrow_format, arguments);
  free(narrow_buffer);
  free(narrow_format);
  return result;
}

int swscanf(const wchar_t* restrict buffer,
            const wchar_t* restrict format, ...) {
  va_list arguments;
  int result;
  va_start(arguments, format);
  result = vswscanf(buffer, format, arguments);
  va_end(arguments);
  return result;
}

static size_t narrow_wide_prefix(const wchar_t* string, char* buffer,
                                 size_t capacity) {
  size_t length = 0;
  while (length + 1 < capacity && string[length] >= 0 &&
         string[length] <= 0x7f) {
    buffer[length] = (char)string[length];
    if (string[length] == L'\0') return length;
    length++;
  }
  buffer[length] = '\0';
  return length;
}

double wcstod(const wchar_t* restrict string, wchar_t** restrict end) {
  char buffer[256];
  char* narrow_end;
  double result;
  narrow_wide_prefix(string, buffer, sizeof(buffer));
  result = strtod(buffer, &narrow_end);
  if (end != NULL) *end = (wchar_t*)string + (narrow_end - buffer);
  return result;
}

float wcstof(const wchar_t* restrict string, wchar_t** restrict end) {
  return (float)wcstod(string, end);
}

long double wcstold(const wchar_t* restrict string, wchar_t** restrict end) {
  return (long double)wcstod(string, end);
}

long wcstol(const wchar_t* restrict string, wchar_t** restrict end, int base) {
  char buffer[256];
  char* narrow_end;
  long result;
  narrow_wide_prefix(string, buffer, sizeof(buffer));
  result = strtol(buffer, &narrow_end, base);
  if (end != NULL) *end = (wchar_t*)string + (narrow_end - buffer);
  return result;
}

long long wcstoll(const wchar_t* restrict string, wchar_t** restrict end,
                  int base) {
  char buffer[256];
  char* narrow_end;
  long long result;
  narrow_wide_prefix(string, buffer, sizeof(buffer));
  result = strtoll(buffer, &narrow_end, base);
  if (end != NULL) *end = (wchar_t*)string + (narrow_end - buffer);
  return result;
}

unsigned long wcstoul(const wchar_t* restrict string,
                      wchar_t** restrict end, int base) {
  char buffer[256];
  char* narrow_end;
  unsigned long result;
  narrow_wide_prefix(string, buffer, sizeof(buffer));
  result = strtoul(buffer, &narrow_end, base);
  if (end != NULL) *end = (wchar_t*)string + (narrow_end - buffer);
  return result;
}

unsigned long long wcstoull(const wchar_t* restrict string,
                            wchar_t** restrict end, int base) {
  char buffer[256];
  char* narrow_end;
  unsigned long long result;
  narrow_wide_prefix(string, buffer, sizeof(buffer));
  result = strtoull(buffer, &narrow_end, base);
  if (end != NULL) *end = (wchar_t*)string + (narrow_end - buffer);
  return result;
}

static int append_wide_text(wchar_t* destination, size_t count, size_t* used,
                            const wchar_t* text) {
  while (*text != L'\0') {
    if (*used + 1 >= count) return 0;
    destination[(*used)++] = *text++;
  }
  return 1;
}

static int append_wide_number(wchar_t* destination, size_t count, size_t* used,
                              int value, int width) {
  wchar_t digits[12];
  int index = width;
  int i;
  if (index < 1) index = 1;
  for (i = 0; i < index; i++) {
    digits[index - i - 1] = (wchar_t)(L'0' + value % 10);
    value /= 10;
  }
  digits[index] = L'\0';
  return append_wide_text(destination, count, used, digits);
}

size_t wcsftime(wchar_t* restrict destination, size_t count,
                const wchar_t* restrict format, const struct tm* restrict time) {
  static const wchar_t* weekdays[] = {
      L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
      L"Thursday", L"Friday", L"Saturday"};
  static const wchar_t* months[] = {
      L"January", L"February", L"March", L"April", L"May", L"June",
      L"July", L"August", L"September", L"October", L"November", L"December"};
  size_t used = 0;
  if (count == 0) return 0;
  while (*format != L'\0') {
    wchar_t conversion;
    const wchar_t* text;
    if (*format != L'%') {
      if (used + 1 >= count) return 0;
      destination[used++] = *format++;
      continue;
    }
    format++;
    conversion = *format++;
    if (conversion == L'%') {
      if (used + 1 >= count) return 0;
      destination[used++] = L'%';
    } else if (conversion == L'Y') {
      if (!append_wide_number(destination, count, &used,
                              time->tm_year + 1900, 4)) return 0;
    } else if (conversion == L'y') {
      if (!append_wide_number(destination, count, &used,
                              (time->tm_year + 1900) % 100, 2)) return 0;
    } else if (conversion == L'm') {
      if (!append_wide_number(destination, count, &used,
                              time->tm_mon + 1, 2)) return 0;
    } else if (conversion == L'd') {
      if (!append_wide_number(destination, count, &used,
                              time->tm_mday, 2)) return 0;
    } else if (conversion == L'H') {
      if (!append_wide_number(destination, count, &used,
                              time->tm_hour, 2)) return 0;
    } else if (conversion == L'M') {
      if (!append_wide_number(destination, count, &used,
                              time->tm_min, 2)) return 0;
    } else if (conversion == L'S') {
      if (!append_wide_number(destination, count, &used,
                              time->tm_sec, 2)) return 0;
    } else if (conversion == L'a' || conversion == L'A') {
      text = time->tm_wday >= 0 && time->tm_wday < 7
                 ? weekdays[time->tm_wday] : L"?";
      if (conversion == L'a') {
        wchar_t short_name[4] = {text[0], text[1], text[2], L'\0'};
        if (!append_wide_text(destination, count, &used, short_name)) return 0;
      } else if (!append_wide_text(destination, count, &used, text)) {
        return 0;
      }
    } else if (conversion == L'b' || conversion == L'B') {
      text = time->tm_mon >= 0 && time->tm_mon < 12
                 ? months[time->tm_mon] : L"?";
      if (conversion == L'b') {
        wchar_t short_name[4] = {text[0], text[1], text[2], L'\0'};
        if (!append_wide_text(destination, count, &used, short_name)) return 0;
      } else if (!append_wide_text(destination, count, &used, text)) {
        return 0;
      }
    } else {
      if (used + 2 >= count) return 0;
      destination[used++] = L'%';
      destination[used++] = conversion;
    }
  }
  destination[used] = L'\0';
  return used;
}
