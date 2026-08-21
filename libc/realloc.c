//
// The modern Realloc implementation lives with the TLSF metadata in malloc.c.
// The 6502 uses a compact allocate/copy/free implementation.
//

#if defined(__6502__)

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define Malloc malloc
#define Free free
#define Realloc realloc

void* Realloc(void* pointer, size_t size) {
  if (pointer == NULL) {
    return Malloc(size);
  }
  if (size == 0) {
    Free(pointer);
    return NULL;
  }

  size_t old_size = *((size_t*)pointer - 1);
  if (size <= old_size) {
    return pointer;
  }
  void* replacement = Malloc(size);
  if (replacement == NULL) {
    return NULL;
  }
  memcpy(replacement, pointer, old_size);
  Free(pointer);
  return replacement;
}

#endif
