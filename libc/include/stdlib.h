//
//  stdlib.h
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//


#ifndef stdlib_h
#define stdlib_h
#ifdef __DAVECC__

#ifdef __cplusplus
extern "C" {
#endif

#define NULL ((void*)0)

#ifndef __SIZE_T
#if defined(__W65C02__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#ifndef __SSIZE_T
#if defined(__W65C02__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define __SSIZE_T
#endif


#ifndef __DIV_T
typedef struct __div_t {
  int quot;
  int rem;
} div_t;
#define __DIV_T
#endif

#ifndef __LDIV_T
typedef struct __ldiv_t {
  long int quot;
  long int rem;
} ldiv_t;
#define __LDIV_T
#endif

#ifndef __LLDIV_T
typedef struct __lldiv_t {
  long long int quot;
  long long int rem;
} lldiv_t;
#define __LLDIV_T
#endif

#if !defined(__WCHAR_T) && !defined(__cplusplus)
typedef int wchar_t;
#define __WCHAR_T
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define RAND_MAX 0xffffffff
#define MB_CUR_MAX 4


double atof(const char *nptr);
int atoi(const char *nptr);
long int atol(const char *nptr);
long long int atoll(const char *nptr);
double strtod(const char * restrict nptr,
     char ** restrict endptr);
float strtof(const char * restrict nptr,
     char ** restrict endptr);
long double strtold(const char * restrict nptr,
     char ** restrict endptr);
long int strtol(const char * restrict nptr,
     char ** restrict endptr, int base);
long long int strtoll(const char * restrict nptr,
     char ** restrict endptr, int base);
unsigned long int strtoul(
     const char * restrict nptr,
     char ** restrict endptr, int base);
unsigned long long int strtoull(
     const char * restrict nptr,
     char ** restrict endptr, int base);
int rand(void);
void srand(unsigned int seed);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
#ifdef __cplusplus
[[noreturn]] void abort(void);
#else
void __attribute__((noreturn)) abort(void);
#endif
int atexit(void (*func)(void));
void exit(int status);
void _Exit(int status);
char *getenv(const char *name);
int system(const char *string);
void *bsearch(const void *key, const void *base,
size_t nmemb, size_t size,
int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
     int (*compar)(const void *, const void *));
int abs(int j);
long int labs(long int j);
long long int llabs(long long int j);
div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
lldiv_t lldiv(long long int numer, long long int denom);
int mblen(const char *s, size_t n);
int mbtowc(wchar_t * restrict pwc,
   const char * restrict s, size_t n);
int wctomb(char *s, wchar_t wchar);
size_t mbstowcs(wchar_t * restrict pwcs,
      const char * restrict s, size_t n);
size_t wcstombs(char * restrict s,
const wchar_t * restrict pwcs, size_t n);
char* realpath(const char* path, char* resolved_path);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC__ */
#endif /* stdlib_h */
