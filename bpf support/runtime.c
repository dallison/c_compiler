#include <stddef.h>

// The allocator in malloc.c is published as Malloc/Free/Realloc when the
// heap lock is on.  This target is one thread, so the lock is a no-op and
// the C names are direct calls.

extern void* Malloc(size_t size);
extern void Free(void* pointer);
extern void* Realloc(void* pointer, size_t size);

void* malloc(size_t size) { return Malloc(size); }

void free(void* pointer) { Free(pointer); }

void* realloc(void* pointer, size_t size) { return Realloc(pointer, size); }

void __davecc_tls_thread_fini(void) {}
