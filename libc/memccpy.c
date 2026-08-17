#include <stddef.h>
#include <string.h>

void* memccpy(void* restrict destination, const void* restrict source, int c,
              size_t count) {
  unsigned char* to = destination;
  const unsigned char* from = source;
  unsigned char byte = (unsigned char)c;
  for (size_t i = 0; i < count; ++i) {
    to[i] = from[i];
    if (from[i] == byte) {
      return to + i + 1;
    }
  }
  return NULL;
}
