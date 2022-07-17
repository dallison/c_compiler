//
//  stdio.h
//  c_compiler
//
//  Created by David Allison on 1/11/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stdio_h
#define stdio_h

#ifdef __DAVECC__
#include <stdarg.h>

#define EOF (-1)
#define NULL ((void*)0)

#ifndef __FPOS_T
#if defined(__W65C02__)
typedef int fpos_t;
#else
typedef long fpos_t;
#endif
#define __FPOS_T
#endif

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

typedef struct {
  int fd;
  char* buf;
  int bufsize;
  int rindex;     // Read index into buf.
  int rlimit;     // Limit of chars to read.
  int windex;     // Write index.
  int buffering_mode;
  char buffer_owned;    // The buffer is owned by this FILE.
  char unget_index;
  char eof_flag;
  char error_flag;
  char unget_buf[10];
} FILE;

// Buffering modes.
#define _IOFBF 1
#define _IOLBF 2
#define _IONBF 3

// Default buffer size.
#if defined(__W65C02__)
#define BUFSIZE 64
#else
#define BUFSIZE 4096
#endif
#define BUFSIZ BUFSIZE

#define FOPEN_MAX 0
#if defined(__W65C02__)
#define FILENAME_MAX 16
#else
#define FILENAME_MAX 256
#endif
#define L_tmpnam 16

#define SEEK_CUR 0
#define SEET_END 1
#define SEEK_SET 2

#define TMP_MAX 256

extern FILE* stdout;
extern FILE* stdin;
extern FILE* stderr;

#if defined(__W65C02__)
typedef char mode_t;
typedef char char_t;
#else
typedef int mode_t;
typedef int char_t;
#endif

int remove(const char *filename);
int rename(const char *old, const char *new);
FILE *tmpfile(void);
char *tmpnam(char *s);
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char * restrict filename,
     const char * restrict mode);
FILE *freopen(const char * restrict filename,
     const char * restrict mode,
     FILE * restrict stream);
void setbuf(FILE * restrict stream,
     char * restrict buf);
int setvbuf(FILE * restrict stream,
     char * restrict buf,
            mode_t mode, size_t size);
int fprintf(FILE * restrict stream,
     const char * restrict format, ...);
int fscanf(FILE * restrict stream,
           const char * restrict format, ...);
int printf(const char * restrict format, ...);
int scanf(const char * restrict format, ...);
int snprintf(char * restrict s, size_t n,
     const char * restrict format, ...);
int sprintf(char * restrict s,
     const char * restrict format, ...);
int sscanf(const char * restrict s,
     const char * restrict format, ...);
int vfprintf(FILE * restrict stream,
             const char * restrict format, va_list arg);
int vfscanf(FILE * restrict stream,
            const char * restrict format, va_list arg);
int vprintf(const char * restrict format, va_list arg);
int vscanf(const char * restrict format, va_list arg);
int vsnprintf(char * restrict s, size_t n,
              const char * restrict format, va_list arg);
int vsprintf(char * restrict s,
             const char * restrict format, va_list arg);
int vsscanf(const char * restrict s,
            const char * restrict format, va_list arg);
int fgetc(FILE *stream);
char *fgets(char * restrict s, int n,
     FILE * restrict stream);
int fputc(char_t c, FILE *stream);
int fputs(const char * restrict s,
     FILE * restrict stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s);
int putchar(char_t c);
int puts(const char *s);
int ungetc(int c, FILE *stream);
size_t fread(void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
FILE * restrict stream);
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fseek(FILE *stream, long int offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long int ftell(FILE *stream);
void rewind(FILE *stream);
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);
char* strerror(int errnum);

#endif /* __DAVECC__ */
#endif /* stdio_h */
