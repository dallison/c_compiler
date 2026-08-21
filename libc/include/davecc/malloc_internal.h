#ifndef DAVECC_MALLOC_INTERNAL_H
#define DAVECC_MALLOC_INTERNAL_H

#include <stddef.h>

typedef struct DaveHeapStats {
  size_t mapped_bytes;
  size_t direct_mapped_bytes;
  size_t allocated_bytes;
  size_t peak_allocated_bytes;
  size_t free_bytes;
  size_t largest_free_block;
  size_t region_count;
  size_t allocation_count;
  size_t free_count;
  size_t bin_lookups;
} DaveHeapStats;

void __davecc_heap_get_stats(DaveHeapStats* stats);
int __davecc_heap_check(void);
size_t __davecc_malloc_cache_class(void* pointer);

#endif
