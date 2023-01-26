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
#include <errno.h>

int fflush(FILE *stream) {
  if (stream == NULL) {
    // Flush all files.
    FILE* fp = __all_files;
    while (fp != NULL) {
      fflush(fp);   // Ignore error if any.
      fp = fp->next;
    }
    return 0;
  }
  
  if (stream->buf != NULL) {
    // Flush any input chars not read.
    stream->rlimit = stream->rindex = 0;
    
    // Flush output to OS.
    ssize_t remaining = stream->windex;           // Bytes remaining to write.
    stream->windex = 0;
    const char* buf = stream->buf;
    
    // The call to write may not write all bytes in one call.
    while (remaining > 0) {
      ssize_t nbytes = remaining;
      ssize_t n = write(stream->fd, buf, remaining);
      if (n < 0) {
        stream->error_flag = 1;
        return EOF;
      }
      // Update for next iteration.
      remaining -= n;
      buf += n;
    }
  }
  return 0;
}
