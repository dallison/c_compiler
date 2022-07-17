//
//  fseek.c
//  c_compiler
//
//  Created by David Allison on 12/22/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>

int fseek(FILE *stream, long int offset, int whence) {
  fflush(stream);
  int e = lseek(stream->fd, offset, whence);
  if (e == -1) {
    return e;
  }
  stream->error_flag = stream->eof_flag = 0;
  return 0;
}

long int ftell(FILE *stream) {
  fflush(stream);
  return lseek(stream->fd, 0, SEEK_CUR);
}

int fgetpos(FILE * restrict stream, fpos_t * restrict pos) {
  long p = ftell(stream);
  if (p == -1) {
    return -1;
  }
  *pos = p;
  return 0;
}

int fsetpos(FILE *stream, const fpos_t *pos) {
  fpos_t p = fseek(stream, *pos, SEEK_SET);
  if (p == -1) {
    return -1;
  }
  return 0;
}

void rewind(FILE *stream) {
  fseek(stream, 0, SEEK_SET);
}
