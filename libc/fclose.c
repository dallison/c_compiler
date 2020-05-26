//
//  fclose.c
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int fclose(FILE* fp) {
  if (fp == NULL) {
    return EOF;
  }
  int e = close(fp->fd);
  free(fp->buf);
  return e == 0 ? 0 : EOF;
}
