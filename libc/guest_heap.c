//
//  guest_heap.c
//  libc
//
//  Thread-safe wrappers around the shared free-list allocator.
//

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>

#include <davecc/malloc_internal.h>

#if defined(__DAVECC_HAS_HEAP_LOCK__)

#if defined(__DAVECC_NATIVE_LINUX__)
static unsigned int native_heap_lock;

static void HeapLock(void) {
  for (;;) {
    unsigned int expected = 0;
    if (__atomic_compare_exchange_n(&native_heap_lock, &expected, 1, 0, 2, 0)) {
      return;
    }
    syscall(SYS_futex, &native_heap_lock, 0, 1, 0, 0, 0);
  }
}

static void HeapUnlock(void) {
  __atomic_store_n(&native_heap_lock, 0, 3);
  syscall(SYS_futex, &native_heap_lock, 1, 1, 0, 0, 0);
}
#else
static void HeapLock(void) { syscall(SYS_HEAP_LOCK); }
static void HeapUnlock(void) { syscall(SYS_HEAP_UNLOCK); }

unsigned long long
    __davecc_guest_heap_storage[((1024 * 1024) + 15) /
                                sizeof(unsigned long long)];
#endif

extern void* Malloc(size_t n);
extern void Free(void* p);
extern void* Realloc(void* p, size_t n);

enum {
  DAVE_HEAP_CACHE_ALIGNMENT = 16,
  DAVE_HEAP_CACHE_MAX_SIZE = 1024,
  DAVE_HEAP_CACHE_CLASSES =
      DAVE_HEAP_CACHE_MAX_SIZE / DAVE_HEAP_CACHE_ALIGNMENT,
  DAVE_HEAP_CACHE_DEPTH = 2,
};

typedef struct DaveHeapThreadCache {
  void* entries[DAVE_HEAP_CACHE_CLASSES][DAVE_HEAP_CACHE_DEPTH];
  unsigned char counts[DAVE_HEAP_CACHE_CLASSES];
} DaveHeapThreadCache;

static __thread DaveHeapThreadCache thread_cache;

static int FlushThreadCacheLocked(void) {
  int released = 0;
  for (int cache_class = 0; cache_class < DAVE_HEAP_CACHE_CLASSES;
       ++cache_class) {
    unsigned char count = thread_cache.counts[cache_class];
    while (count != 0) {
      Free(thread_cache.entries[cache_class][--count]);
      released = 1;
    }
    thread_cache.counts[cache_class] = 0;
  }
  return released;
}

static int CacheClassForRequest(size_t size) {
  if (size > DAVE_HEAP_CACHE_MAX_SIZE) {
    return -1;
  }
  size_t rounded =
      (size + DAVE_HEAP_CACHE_ALIGNMENT - 1) &
      ~(size_t)(DAVE_HEAP_CACHE_ALIGNMENT - 1);
  if (rounded == 0) {
    rounded = DAVE_HEAP_CACHE_ALIGNMENT;
  }
  return (int)(rounded / DAVE_HEAP_CACHE_ALIGNMENT) - 1;
}

void* malloc(size_t n) {
  int cache_class = CacheClassForRequest(n);
  if (cache_class >= 0 && thread_cache.counts[cache_class] != 0) {
    unsigned char count = --thread_cache.counts[cache_class];
    return thread_cache.entries[cache_class][count];
  }
  HeapLock();
  void* p = Malloc(n);
  if (p == NULL && FlushThreadCacheLocked()) {
    p = Malloc(n);
  }
  HeapUnlock();
  return p;
}

void free(void* p) {
  if (p == NULL) {
    return;
  }
  size_t capacity = __davecc_malloc_cache_class(p);
  if (capacity != 0) {
    int cache_class =
        (int)(capacity / DAVE_HEAP_CACHE_ALIGNMENT) - 1;
    unsigned char count = thread_cache.counts[cache_class];
    if (count < DAVE_HEAP_CACHE_DEPTH) {
      thread_cache.entries[cache_class][count] = p;
      thread_cache.counts[cache_class] = count + 1;
      return;
    }
  }
  HeapLock();
  Free(p);
  HeapUnlock();
}

void* realloc(void* p, size_t n) {
  HeapLock();
  void* result = Realloc(p, n);
  HeapUnlock();
  return result;
}

void __davecc_heap_thread_cleanup(void) {
  int has_entries = 0;
  for (int cache_class = 0; cache_class < DAVE_HEAP_CACHE_CLASSES;
       ++cache_class) {
    if (thread_cache.counts[cache_class] != 0) {
      has_entries = 1;
      break;
    }
  }
  if (!has_entries) {
    return;
  }

  HeapLock();
  FlushThreadCacheLocked();
  HeapUnlock();
}

#endif
