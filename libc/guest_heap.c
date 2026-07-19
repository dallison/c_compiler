//
//  guest_heap.c
//  libc
//
//  Thread-safe wrappers around the guest allocator.
//

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>

#if defined(__DAVECC_HAS_HEAP_LOCK__)

extern void* Malloc(size_t n);
extern void Free(void* p);
extern void* Realloc(void* p, size_t n);

void* malloc(size_t n) {
  syscall(SYS_HEAP_LOCK);
  void* p = Malloc(n);
  syscall(SYS_HEAP_UNLOCK);
  return p;
}

void free(void* p) {
  if (p == NULL) {
    return;
  }
  syscall(SYS_HEAP_LOCK);
  Free(p);
  syscall(SYS_HEAP_UNLOCK);
}

void* realloc(void* p, size_t n) {
  syscall(SYS_HEAP_LOCK);
  void* result = Realloc(p, n);
  syscall(SYS_HEAP_UNLOCK);
  return result;
}

#endif
