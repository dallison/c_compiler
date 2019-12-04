//
//  printf.c
//  c_compiler
//
//  Created by David Allison on 5/5/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdarg.h>
#include <stdbool.h>

int write(int fp, const char* buf, int len) {
#if defined(__risc_v__)
  // The ecall instruction is used with r0 set to 4.  This
  // is the write system call.
  return asm(
      "li t6, 4\n"
      "ecall\n"
      );
#elif defined(__p_code__)
  // This uses escape code 2 to call the write system call.
  // Stack contains arguments.
  // offset 28: length
  // offset 20: buffer
  // offset 16: file descriptor
  return asm(
             "pushx r1\n"
             "pushx r2\n"
             "ldw r0, [ap, #16]\n"
             "ldx r1, [ap, #20]\n"
             "ldw r2, [ap, #28]\n"
             "esc #2\n"
             "popx r2\n"
             "popx r1\n"
  );
#endif
}

#define NULL 0
#define EOF (-1)

typedef struct {
  int fd;
  char* buf;
  int bufsize;
  int index;
} FILE;

#define STDOUT_BUFSIZE 4096
#define STDIN_BUFSIZE 4096

static char stdout_buf[STDOUT_BUFSIZE];
static char stdin_buf[STDIN_BUFSIZE];

static FILE s_stdin = {0, stdin_buf, sizeof(stdin_buf), 0};
static FILE s_stdout = {1, stdout_buf, sizeof(stdout_buf), 0};
static FILE s_stderr = {2, NULL, 0, 0};

FILE* stdout = &s_stdout;
FILE* stdin = &s_stdin;
FILE* stderr = &s_stderr;

int fflush(FILE* fp) {
  int e = write(fp->fd, fp->buf, fp->index);
  if (e > 0) {
    fp->index = 0;
    return 0;
  }
  return EOF;
}

int fputc(int c, FILE* fp) {
  if (fp->buf == NULL) {
    int e = write(fp->fd, &fp->buf, 1);
    return e == 0 ? 0 : EOF;
  }
  // Buffer full?
  if (fp->index == fp->bufsize) {
    int e = fflush(fp);
    if (e != 0) {
      return e;
    }
  }
  // Add to next position in buffer.
  fp->buf[fp->index++] = c;
  
  if (c == '\n') {
    // Flush on newline.
    return fflush(fp);
  }
  return 0;
}

int putchar(char c) {
  return fputc(c, stdout);
}

int fputs(const char* str, FILE* fp) {
  const char* s = str;
  while (*s != '\0') {
    fputc(*s++, fp);
  }
}

static void PrintInt(long long v, FILE* fp) {
  char buf[16];
  int i = 0;
  if (v == 0) {
    fputc('0', fp);
    return;
  }
  bool negative = false;
  if (v < 0) {
    negative = true;
    v = -v;
  }
  while (v != 0) {
    char ch = (v % 10) + '0';
    buf[i++] = ch;
    v /= 10;
  }
  if (negative) {
    buf[i++] = '-';
  }
  // Digits in buf are reversed.
  i--;
  while (i >= 0) {
    fputc(buf[i--], fp);
  }
}

int vfprintf(FILE* fp, const char* format, va_list ap) {
  const char* p = format;
  while (*p != '\0') {
    if (*p == '%') {
      p++;
      switch (*p) {
        case 'd':
          p++;
          PrintInt(va_arg(ap, int), fp);
          break;
        case 's':
          p++;
          fputs(va_arg(ap, char*), fp);
          break;
        default:
          fputc(*p++, fp);
          break;
      }
    } else {
      fputc(*p++, fp);
    }
  }
}

int fprintf(FILE* fp, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(fp, format, ap);
  va_end(ap);
}

int printf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
}

int main(int argc, char** argv) {
  // printf("%d\n", 1234);
  for (int i = 100; i > 13; i-= 8) {
    printf("hello %s %d\n", "world", -1234*i);
    //printf("%d\n", -i);
  }
  //fputs("from fputs\n", stdout);
}
