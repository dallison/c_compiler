// C wide-character and multibyte support.

#ifndef wchar_h
#define wchar_h

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#if !defined(__WCHAR_T) && !defined(__cplusplus)
typedef __WCHAR_TYPE__ wchar_t;
#define __WCHAR_T
#endif

#ifndef __WINT_T
typedef __WINT_TYPE__ wint_t;
#define __WINT_T
#endif

#ifndef __MBSTATE_T
typedef struct __davecc_mbstate_t {
  unsigned int __value;
  unsigned short __pending;
  unsigned char __count;
  unsigned char __expected;
  unsigned char __has_pending;
} mbstate_t;
#define __MBSTATE_T
#endif

#if defined(__6502__) || defined(__W65C02__)
#define WCHAR_MIN (-32767 - 1)
#define WCHAR_MAX 32767
#else
#define WCHAR_MIN (-2147483647 - 1)
#define WCHAR_MAX 2147483647
#endif
#define WEOF ((wint_t)-1)

struct tm;

#ifdef __cplusplus
extern "C" {
#endif

int fwprintf(FILE* restrict stream, const wchar_t* restrict format, ...);
int fwscanf(FILE* restrict stream, const wchar_t* restrict format, ...);
int swprintf(wchar_t* restrict buffer, size_t size,
             const wchar_t* restrict format, ...);
int swscanf(const wchar_t* restrict buffer,
            const wchar_t* restrict format, ...);
int vfwprintf(FILE* restrict stream, const wchar_t* restrict format,
              va_list arguments);
int vfwscanf(FILE* restrict stream, const wchar_t* restrict format,
             va_list arguments);
int vswprintf(wchar_t* restrict buffer, size_t size,
              const wchar_t* restrict format, va_list arguments);
int vswscanf(const wchar_t* restrict buffer,
             const wchar_t* restrict format, va_list arguments);

wint_t fgetwc(FILE* stream);
wchar_t* fgetws(wchar_t* restrict buffer, int count, FILE* restrict stream);
wint_t fputwc(wchar_t value, FILE* stream);
int fputws(const wchar_t* restrict string, FILE* restrict stream);
int fwide(FILE* stream, int mode);
wint_t getwc(FILE* stream);
wint_t getwchar(void);
wint_t putwc(wchar_t value, FILE* stream);
wint_t putwchar(wchar_t value);
wint_t ungetwc(wint_t value, FILE* stream);

double wcstod(const wchar_t* restrict string, wchar_t** restrict end);
float wcstof(const wchar_t* restrict string, wchar_t** restrict end);
long double wcstold(const wchar_t* restrict string, wchar_t** restrict end);
long wcstol(const wchar_t* restrict string, wchar_t** restrict end, int base);
long long wcstoll(const wchar_t* restrict string, wchar_t** restrict end,
                  int base);
unsigned long wcstoul(const wchar_t* restrict string,
                      wchar_t** restrict end, int base);
unsigned long long wcstoull(const wchar_t* restrict string,
                            wchar_t** restrict end, int base);

wchar_t* wcscpy(wchar_t* restrict destination,
                 const wchar_t* restrict source);
wchar_t* wcsncpy(wchar_t* restrict destination,
                  const wchar_t* restrict source, size_t count);
wchar_t* wmemcpy(wchar_t* restrict destination,
                  const wchar_t* restrict source, size_t count);
wchar_t* wmemmove(wchar_t* destination, const wchar_t* source, size_t count);
wchar_t* wcscat(wchar_t* restrict destination,
                 const wchar_t* restrict source);
wchar_t* wcsncat(wchar_t* restrict destination,
                  const wchar_t* restrict source, size_t count);
int wcscmp(const wchar_t* left, const wchar_t* right);
int wcscoll(const wchar_t* left, const wchar_t* right);
int wcsncmp(const wchar_t* left, const wchar_t* right, size_t count);
int wmemcmp(const wchar_t* left, const wchar_t* right, size_t count);
size_t wcsxfrm(wchar_t* restrict destination,
               const wchar_t* restrict source, size_t count);
wchar_t* wcschr(const wchar_t* string, wchar_t value);
size_t wcscspn(const wchar_t* string, const wchar_t* reject);
wchar_t* wcspbrk(const wchar_t* string, const wchar_t* accept);
wchar_t* wcsrchr(const wchar_t* string, wchar_t value);
size_t wcsspn(const wchar_t* string, const wchar_t* accept);
wchar_t* wcsstr(const wchar_t* string, const wchar_t* substring);
wchar_t* wcstok(wchar_t* restrict string, const wchar_t* restrict delimiters,
                wchar_t** restrict state);
wchar_t* wmemchr(const wchar_t* memory, wchar_t value, size_t count);
size_t wcslen(const wchar_t* string);
wchar_t* wmemset(wchar_t* destination, wchar_t value, size_t count);

size_t wcsftime(wchar_t* restrict destination, size_t count,
                const wchar_t* restrict format, const struct tm* restrict time);

int mbsinit(const mbstate_t* state);
size_t mbrlen(const char* restrict string, size_t count,
              mbstate_t* restrict state);
size_t mbrtowc(wchar_t* restrict output, const char* restrict string,
               size_t count, mbstate_t* restrict state);
size_t wcrtomb(char* restrict output, wchar_t value,
               mbstate_t* restrict state);
size_t mbsrtowcs(wchar_t* restrict output, const char** restrict source,
                 size_t count, mbstate_t* restrict state);
size_t wcsrtombs(char* restrict output, const wchar_t** restrict source,
                 size_t count, mbstate_t* restrict state);

#ifdef __cplusplus
}
#endif

#endif /* wchar_h */
