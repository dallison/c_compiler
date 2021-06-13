//
//  fputc.c
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

static FILE s_stdin = {0, NULL, BUFSIZE, 0, _IOLBF};
static FILE s_stdout = {1, NULL, BUFSIZE, 0, _IOLBF};
static FILE s_stderr = {2, NULL, 0, 0, _IONBF};

FILE* stdin = &s_stdin;
FILE* stdout = &s_stdout;
FILE* stderr = &s_stderr;

static void EnsureBuffer(FILE* fp) {
  if (fp->buffering_mode != _IONBF) {
    if (fp->buf == NULL) {
      fp->buf = malloc(fp->bufsize);
    }
  }
}

void setbuf(FILE * restrict stream,
            char * restrict buf) {
  setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

int setvbuf(FILE * restrict stream,
     char * restrict buf,
            int mode, size_t size) {
  if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
    return -1;
  }
  if (stream->buf != NULL) {
    free(stream->buf);
  }
  stream->buf = buf;
  stream->bufsize = size;
  stream->buffering_mode = mode;
  return 0;
}

size_t fwrite(const void*  ptr, size_t size, size_t n, FILE* stream) {
  char* p = ptr;
  size_t len = size * n;
  int numchars = 0;
  while (len > 0) {
    int v = fputc(*p++, stream);
    if (v == EOF) {
      return n;
    }
    numchars++;
    len--;
  }
  return numchars;
}


int fputc(int c, FILE* fp) {
  EnsureBuffer(fp);
  if (fp->buf == NULL) {
    char ch[1] = {c};
    int e = write(fp->fd, ch, 1);
    return e == 1 ? c : EOF;
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
  
  if (c == '\n' && fp->buffering_mode == _IOLBF) {
    // Flush on newline.
    return fflush(fp);
  }
  return 0;
}

int putchar(int c) {
  return fputc(c, stdout);
}

int fputs(const char* str, FILE* fp) {
  const char* s = str;
  while (*s != '\0') {
    if (fputc(*s++, fp) == EOF) {
      return EOF;
    }
  }
  return 0;
}

int fflush(FILE *stream) {
  EnsureBuffer(stream);
  if (stream->buf != NULL) {
    int index = stream->index;
    stream->index = 0;
    int e = write(stream->fd, stream->buf, index);
    if (e == -1) {
      return EOF;
    }
  }
  return 0;
}

int puts(const char* str) {
  return fputs(str, stdout);
}
//size_t fread(char* ptr, size_t size, size_t n, FILE* stream) {
  
//}
