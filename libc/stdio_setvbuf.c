// setbuf/setvbuf live apart from the FILE globals (stdio.c) so programs that
// never adjust buffering don't link the buffer management code.

#include <stdio.h>
#include <stdlib.h>

void setbuf(FILE* restrict stream, char* restrict buf) {
  setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

int setvbuf(FILE* restrict stream, char* restrict buf, mode_t mode,
            size_t size) {
  if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
    return -1;
  }
  if (stream->buf != NULL && stream->buffer_owned) {
    free(stream->buf);
  }
  if (buf == NULL && mode != _IONBF) {
    stream->buf = malloc(size);
    stream->buffer_owned = 1;
  } else {
    stream->buf = buf;
    stream->buffer_owned = 0;
  }
  stream->bufsize = size;
  stream->buffering_mode = mode;
  return 0;
}
