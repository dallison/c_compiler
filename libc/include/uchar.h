#ifndef uchar_h
#define uchar_h

#include <stddef.h>
#include <wchar.h>

#ifndef __cplusplus
typedef __CHAR16_TYPE__ char16_t;
typedef __CHAR32_TYPE__ char32_t;
typedef unsigned char char8_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

size_t mbrtoc16(char16_t* restrict output, const char* restrict string,
                size_t count, mbstate_t* restrict state);
size_t c16rtomb(char* restrict output, char16_t value,
                mbstate_t* restrict state);
size_t mbrtoc32(char32_t* restrict output, const char* restrict string,
                size_t count, mbstate_t* restrict state);
size_t c32rtomb(char* restrict output, char32_t value,
                mbstate_t* restrict state);
#if !defined(__cplusplus) || __cplusplus >= 202002L
size_t mbrtoc8(char8_t* restrict output, const char* restrict string,
               size_t count, mbstate_t* restrict state);
size_t c8rtomb(char* restrict output, char8_t value,
               mbstate_t* restrict state);
#endif

#ifdef __cplusplus
}
#endif

#endif /* uchar_h */
