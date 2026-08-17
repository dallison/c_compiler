#include <stddef.h>
#include <string.h>

void* memset_explicit(void* destination, int c, size_t count) {
  volatile unsigned char* bytes = destination;
  for (size_t i = 0; i < count; ++i) {
    bytes[i] = (unsigned char)c;
  }
  return destination;
}
