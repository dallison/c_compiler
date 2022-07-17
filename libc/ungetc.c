//
//  ungetc.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int ungetc(int ch, FILE* stream) {
  if (ch == EOF) {
    return EOF;
  }
  if (stream->unget_index == sizeof(stream->unget_buf)) {
    return EOF;
  }
  stream->eof_flag = 0;
  stream->unget_buf[stream->unget_index++] = ch;
  return ch;
}

