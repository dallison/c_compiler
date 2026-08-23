//
//  fseek.c
//  c_compiler
//
//  Created by David Allison on 12/22/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>

int fseek(FILE *stream, long int offset, int whence) {
  if (stream->windex != 0 && fflush(stream) == EOF) {
    return -1;
  }
  if (whence == SEEK_CUR) {
    offset -= (stream->rlimit - stream->rindex) + stream->unget_index;
  }
  long e = lseek(stream->fd, offset, whence);
  if (e == -1) {
    return -1;
  }
  stream->rindex = 0;
  stream->rlimit = 0;
  stream->eof_flag = 0;
  stream->unget_index = 0;
  return 0;
}

long int ftell(FILE *stream) {
  if (stream->windex != 0 && fflush(stream) == EOF) {
    return -1;
  }
  long position = lseek(stream->fd, 0, SEEK_CUR);
  if (position == -1) {
    return -1;
  }
  return position - (stream->rlimit - stream->rindex) - stream->unget_index;
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
  int e = fseek(stream, 0, SEEK_SET);
  if (e == 0) {
    stream->eof_flag = 0;
  }
}
