//
//  string.h
//  c_compiler
//
//  Created by David Allison on 1/19/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef string_h
#define string_h
#ifdef __DAVECC__

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STRING_H__ 202311L
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SIZE_T
#if defined(__6502__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#ifndef __SSIZE_T
#if defined(__6502__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define __SSIZE_T
#endif

#define NULL ((void*)0)

void *memcpy(void * restrict s1,
const void * restrict s2, size_t n);
void *memccpy(void * restrict s1,
const void * restrict s2, int c, size_t n);
void *memmove(void *s1, const void *s2, size_t n);
char *strcpy(char * restrict s1,
          const char * restrict s2);
char *strncpy(char * restrict s1,
              const char * restrict s2, size_t n);
char *strcat(char * restrict s1,
          const char * restrict s2);
char *strncat(char * restrict s1,
const char * restrict s2, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strcasecmp(const char *s1, const char *s2);
int strcoll(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int strncasecmp(const char *s1, const char *s2, size_t n);
size_t strxfrm(char * restrict s1,
const char * restrict s2, size_t n);
void *memchr(const void *s, int c, size_t n);
char *strchr(const char *s, int c);
size_t strcspn(const char *s1, const char *s2);
char *strpbrk(const char *s1, const char *s2);
char *strrchr(const char *s, int c);
size_t strspn(const char *s1, const char *s2);
char *strstr(const char *s1, const char *s2);
char *strtok(char * restrict s1, const char * restrict s2);
char *strtok_r(char * restrict s, const char * restrict delimiters,
               char ** restrict save);
void *memset(void *s, int c, size_t n);
void *memset_explicit(void *s, int c, size_t n);
char *strerror(int errnum);
size_t strlen(const char *s);
char *strdup(const char *s);
char *strndup(const char *s, size_t n);

#if !defined(__cplusplus) && defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 202311L
#include <__davecc_const_generic.h>
#define memchr(s, c, n) \
  __DAVECC_CONST_GENERIC((s), const void*, (memchr)((s), (c), (n)))
#define strchr(s, c) \
  __DAVECC_CONST_GENERIC((s), const char*, (strchr)((s), (c)))
#define strpbrk(s1, s2) \
  __DAVECC_CONST_GENERIC((s1), const char*, (strpbrk)((s1), (s2)))
#define strrchr(s, c) \
  __DAVECC_CONST_GENERIC((s), const char*, (strrchr)((s), (c)))
#define strstr(s1, s2) \
  __DAVECC_CONST_GENERIC((s1), const char*, (strstr)((s1), (s2)))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC__ */
#endif /* string_h */
