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

#ifdef __cplusplus
extern "C" {
#endif

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

#if defined(__W65C02__)
typedef char mode_t;
typedef char char_t;
#else
typedef int mode_t;
typedef int char_t;
#endif

typedef struct FILE {
  int fd;         // OS file descriptor.
  char* buf;      // Buffer (or NULL).
  int bufsize;    // Buffer size (or 0).
  int rindex;     // Read index into buf.
  int rlimit;     // Limit of chars to read.
  int windex;     // Write index.
  mode_t buffering_mode;   // _IOFBF, _IOLBF or _IONBF
  char buffer_owned;    // The buffer is owned by this FILE.
  char unget_index;     // Index into unget_buf for ungotten bytes.
  char eof_flag;        // 1 if EOF reached.
  char error_flag;      // 1 for error condition.
  char unget_buf[10];   // Buffer for unget.
  struct FILE* prev;
  struct FILE* next;
} FILE;

// Buffering modes.
#define _IOFBF 1        // Full buffering.
#define _IOLBF 2        // Line buffering.
#define _IONBF 3        // No buffering.

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

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEET_END 2

#define TMP_MAX 256

extern FILE* stdout;
extern FILE* stdin;
extern FILE* stderr;

extern FILE* __all_files;
extern FILE* __last_file;


int remove(const char *filename);
int rename(const char *old, const char *newpath);
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

#ifdef __cplusplus
}
#endif

#endif
#endif /* stdio_h */
