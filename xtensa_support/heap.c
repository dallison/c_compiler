#include <stddef.h>

extern void* Malloc(size_t size);
extern void Free(void* pointer);
extern void* Realloc(void* pointer, size_t size);

unsigned long long
    __davecc_guest_heap_storage[((256 * 1024) + 15) /
                                sizeof(unsigned long long)];

void* malloc(size_t size) { return Malloc(size); }

void free(void* pointer) { Free(pointer); }

void* realloc(void* pointer, size_t size) {
  return Realloc(pointer, size);
}

void __davecc_tls_thread_fini(void) {}
