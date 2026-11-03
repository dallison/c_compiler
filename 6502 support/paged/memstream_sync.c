#include <stdio.h>

// Image 0 has fflush, and fflush asks whether the stream is an
// open_memstream. This bank does not contain open_memstream, so none of
// its FILE objects are memory streams.
int __davecc_memstream_sync(FILE* stream) {
  (void)stream;
  return 0;
}
