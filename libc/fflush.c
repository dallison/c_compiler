//
//  fflush.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>


int fflush(FILE *stream) {
  if (stream->buf != NULL) {
    int index = stream->windex;
    stream->windex = 0;
    int e = write(stream->fd, stream->buf, index);
    if (e <= 0) {
      if (e == 0) {
        stream->eof_flag = 1;
      } else {
        stream->error_flag = 1;
      }
      return EOF;
    }
  }
  return 0;
}
