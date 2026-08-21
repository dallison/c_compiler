#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void* aligned_alloc(size_t alignment, size_t size) {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0 ||
      size % alignment != 0) {
    return NULL;
  }
  size_t natural_alignment =
#if defined(__6502__)
      sizeof(size_t);
#else
      16;
#endif
  if (alignment <= natural_alignment) {
    return malloc(size);
  }
  if (size > (size_t)-1 - (alignment - 1) - 2 * sizeof(size_t)) {
    return NULL;
  }
  void* raw = malloc(size + alignment - 1 + 2 * sizeof(size_t));
  if (raw == NULL) {
    return NULL;
  }
  uintptr_t candidate = (uintptr_t)raw + 2 * sizeof(size_t);
  uintptr_t aligned = (candidate + alignment - 1) & ~(uintptr_t)(alignment - 1);
  ((size_t*)aligned)[-2] = (size_t)(uintptr_t)raw;
  ((size_t*)aligned)[-1] = 1;
  return (void*)aligned;
}

void free_sized(void* pointer, size_t size) {
  (void)size;
  free(pointer);
}

void free_aligned_sized(void* pointer, size_t alignment, size_t size) {
  (void)alignment;
  (void)size;
  free(pointer);
}
