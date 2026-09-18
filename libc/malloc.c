//
//  malloc.c
//  c_compiler
//
//  The 32/64-bit allocator is a two-level segregated-fit allocator.  Free
//  blocks are indexed by size in bitmap-selected bins and carry physical
//  boundary information, so lookup and coalescing do not walk the heap.
//  The 6502 keeps the compact first-fit allocator below.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#if defined(__6502__)

#include "6502/_malloc.h"

#define Malloc malloc
#define Free free
#define Realloc realloc

#define MEMTOP 0xc000

extern char _end[];

FreeBlockHeader* __free_list;
int __initial_heap_size;
static unsigned char __heap_ready;

static void InitFreeList(void) {
  __heap_ready = 1;
  __free_list = (FreeBlockHeader*)_end;
  __free_list->length = MEMTOP - (int)_end;
  __free_list->next = NULL;
}

void* Malloc(size_t size) {
  size_t minimum_payload = sizeof(FreeBlockHeader) - sizeof(size_t);
  if (size < minimum_payload) {
    size = minimum_payload;
  }
  if (size > (size_t)-1 - sizeof(size_t)) {
    return NULL;
  }
  size_t full_size = size + sizeof(size_t);
  if (__free_list == NULL) {
    if (__heap_ready) {
      return NULL;
    }
    InitFreeList();
  }

  FreeBlockHeader* previous = NULL;
  FreeBlockHeader* block = __free_list;
  while (block != NULL) {
    if (block->length >= full_size) {
      size_t remaining = block->length - full_size;
      if (remaining >= sizeof(FreeBlockHeader)) {
        FreeBlockHeader* next =
            (FreeBlockHeader*)((char*)block + full_size);
        next->length = remaining;
        next->next = block->next;
        if (previous == NULL) {
          __free_list = next;
        } else {
          previous->next = next;
        }
      } else {
        size = block->length - sizeof(size_t);
        if (previous == NULL) {
          __free_list = block->next;
        } else {
          previous->next = block->next;
        }
      }
      block->length = size;
      return (char*)block + sizeof(size_t);
    }
    previous = block;
    block = block->next;
  }
  return NULL;
}

#else

#include <stdio.h>

#include <davecc/malloc_internal.h>

#if !defined(__DAVECC__) && !defined(__DAVECC_NATIVE_LINUX__)
#include <errno.h>
#include <sys/mman.h>
#endif

// With no heap lock there is no wrapper to serialize the allocator, so it
// answers to the library's own names rather than being hidden behind one.
#if defined(__p_code__) || defined(__wasm32__)
#define Malloc malloc
#define Free free
#define Realloc realloc
#endif

enum {
  DAVE_HEAP_ALIGNMENT = 16,
  DAVE_HEAP_SL_BITS = 3,
  DAVE_HEAP_SL_COUNT = 1 << DAVE_HEAP_SL_BITS,
  DAVE_HEAP_DEFAULT_SIZE = 1024 * 1024,
  DAVE_HEAP_PAGE_SIZE = 4096,
  DAVE_HEAP_LARGE_THRESHOLD = 256 * 1024,
};

#define DAVE_HEAP_FL_COUNT (sizeof(size_t) * 8)
#define DAVE_BLOCK_FREE ((size_t)1)
#define DAVE_BLOCK_SENTINEL ((size_t)2)
#define DAVE_BLOCK_DIRECT ((size_t)4)
#define DAVE_BLOCK_FLAG_MASK ((size_t)(DAVE_HEAP_ALIGNMENT - 1))
#define DAVE_BLOCK_HEADER_SIZE ((size_t)DAVE_HEAP_ALIGNMENT)
#define DAVE_REGION_HEADER_SIZE ((size_t)DAVE_HEAP_ALIGNMENT)
#define DAVE_MIN_BLOCK_SIZE                                                \
  ((size_t)((DAVE_BLOCK_HEADER_SIZE + 2 * sizeof(void*) +                 \
             DAVE_HEAP_ALIGNMENT - 1) &                                  \
            ~(DAVE_HEAP_ALIGNMENT - 1)))
#define DAVE_ALIGNED_MARKER ((size_t)1)

#if defined(__DAVECC_NATIVE_LINUX__)
#if defined(__arm__)
#define DAVE_HEAP_MMAP SYS_mmap2
#else
#define DAVE_HEAP_MMAP SYS_mmap
#endif
#endif

typedef struct DaveBlock DaveBlock;

struct DaveBlock {
  size_t size_and_flags;
  size_t previous_size;
};

typedef struct DaveFreeLinks {
  DaveBlock* previous;
  DaveBlock* next;
} DaveFreeLinks;

typedef struct DaveRegion {
  struct DaveRegion* next;
  size_t size;
} DaveRegion;

typedef struct DaveHeap {
  size_t first_level_bitmap;
  unsigned int second_level_bitmaps[DAVE_HEAP_FL_COUNT];
  DaveBlock* bins[DAVE_HEAP_FL_COUNT][DAVE_HEAP_SL_COUNT];
  DaveRegion* regions;
  int initialized;
  DaveHeapStats stats;
} DaveHeap;

static DaveHeap heap;
int __initial_heap_size;

void Free(void* pointer);

static size_t AlignDown(size_t value, size_t alignment) {
  return value & ~(alignment - 1);
}

static int AlignUpChecked(size_t value, size_t alignment, size_t* result) {
  if (value > (size_t)-1 - (alignment - 1)) {
    return 0;
  }
  *result = (value + alignment - 1) & ~(alignment - 1);
  return 1;
}

static uintptr_t AlignPointer(uintptr_t value) {
  return (value + DAVE_HEAP_ALIGNMENT - 1) &
         ~(uintptr_t)(DAVE_HEAP_ALIGNMENT - 1);
}

static size_t BlockSize(const DaveBlock* block) {
  return block->size_and_flags & ~DAVE_BLOCK_FLAG_MASK;
}

static int BlockIsFree(const DaveBlock* block) {
  return (block->size_and_flags & DAVE_BLOCK_FREE) != 0;
}

static int BlockIsSentinel(const DaveBlock* block) {
  return (block->size_and_flags & DAVE_BLOCK_SENTINEL) != 0;
}

static int BlockIsDirect(const DaveBlock* block) {
  return (block->size_and_flags & DAVE_BLOCK_DIRECT) != 0;
}

static void SetBlock(DaveBlock* block, size_t size, size_t flags) {
  block->size_and_flags = size | flags;
}

static DaveBlock* NextPhysicalBlock(DaveBlock* block) {
  return (DaveBlock*)((char*)block + BlockSize(block));
}

static DaveBlock* PreviousPhysicalBlock(DaveBlock* block) {
  return (DaveBlock*)((char*)block - block->previous_size);
}

static DaveFreeLinks* FreeLinks(DaveBlock* block) {
  return (DaveFreeLinks*)((char*)block + DAVE_BLOCK_HEADER_SIZE);
}

static void* BlockUserPointer(DaveBlock* block) {
  return (char*)block + DAVE_BLOCK_HEADER_SIZE;
}

static void InitializeUserMarker(DaveBlock* block) {
  if (2 * sizeof(size_t) < DAVE_BLOCK_HEADER_SIZE) {
    ((size_t*)BlockUserPointer(block))[-1] = 0;
  }
}

static DaveBlock* UserPointerBlock(void* pointer) {
  return (DaveBlock*)((char*)pointer - DAVE_BLOCK_HEADER_SIZE);
}

static unsigned FloorLog2(size_t value) {
#if defined(__DAVECC__)
  return (unsigned)(sizeof(size_t) * 8 - 1 -
                    __davecc_clz(value, sizeof(size_t) * 8));
#else
  if (sizeof(size_t) == 8) {
    return (unsigned)(63 - __builtin_clzll((unsigned long long)value));
  }
  return (unsigned)(31 - __builtin_clz((unsigned int)value));
#endif
}

static unsigned FirstSetBitSize(size_t value) {
#if defined(__DAVECC__)
  return (unsigned)__davecc_ctz(value, sizeof(size_t) * 8);
#else
  if (sizeof(size_t) == 8) {
    return (unsigned)__builtin_ctzll((unsigned long long)value);
  }
  return (unsigned)__builtin_ctz((unsigned int)value);
#endif
}

static unsigned FirstSetBitUnsigned(unsigned int value) {
#if defined(__DAVECC__)
  return (unsigned)__davecc_ctz(value, sizeof(value) * 8);
#else
  return (unsigned)__builtin_ctz(value);
#endif
}

static void MapBlockSize(size_t size, unsigned* first, unsigned* second) {
  size_t units = size / DAVE_HEAP_ALIGNMENT;
  unsigned first_index = FloorLog2(units);
  size_t base = (size_t)1 << first_index;
  unsigned shift = first_index >= DAVE_HEAP_SL_BITS
                       ? first_index - DAVE_HEAP_SL_BITS
                       : 0;
  size_t step = (size_t)1 << shift;
  unsigned second_index = (unsigned)((units - base) >> shift);
  if (second_index >= DAVE_HEAP_SL_COUNT) {
    second_index = DAVE_HEAP_SL_COUNT - 1;
  }
  *first = first_index;
  *second = second_index;
}

static int MapSearchSize(size_t size, unsigned* first, unsigned* second) {
  size_t units = size / DAVE_HEAP_ALIGNMENT;
  unsigned first_index = FloorLog2(units);
  unsigned shift = first_index >= DAVE_HEAP_SL_BITS
                       ? first_index - DAVE_HEAP_SL_BITS
                       : 0;
  size_t step = (size_t)1 << shift;
  if (units > (size_t)-1 - (step - 1)) {
    return 0;
  }
  units = (units + step - 1) & ~(step - 1);
  if (units > (size_t)-1 / DAVE_HEAP_ALIGNMENT) {
    return 0;
  }
  MapBlockSize(units * DAVE_HEAP_ALIGNMENT, first, second);
  return 1;
}

static void InsertFreeBlock(DaveBlock* block) {
  unsigned first;
  unsigned second;
  MapBlockSize(BlockSize(block), &first, &second);
  DaveFreeLinks* links = FreeLinks(block);
  DaveBlock* head = heap.bins[first][second];
  links->previous = NULL;
  links->next = head;
  if (head != NULL) {
    FreeLinks(head)->previous = block;
  }
  heap.bins[first][second] = block;
  heap.second_level_bitmaps[first] |= 1u << second;
  heap.first_level_bitmap |= (size_t)1 << first;
}

static void RemoveFreeBlockFromBin(DaveBlock* block, unsigned first,
                                   unsigned second) {
  DaveFreeLinks* links = FreeLinks(block);
  if (links->previous == NULL) {
    heap.bins[first][second] = links->next;
  } else {
    FreeLinks(links->previous)->next = links->next;
  }
  if (links->next != NULL) {
    FreeLinks(links->next)->previous = links->previous;
  }
  if (heap.bins[first][second] == NULL) {
    heap.second_level_bitmaps[first] &= ~(1u << second);
    if (heap.second_level_bitmaps[first] == 0) {
      heap.first_level_bitmap &= ~((size_t)1 << first);
    }
  }
}

static void RemoveFreeBlock(DaveBlock* block) {
  unsigned first;
  unsigned second;
  MapBlockSize(BlockSize(block), &first, &second);
  RemoveFreeBlockFromBin(block, first, second);
}

static DaveBlock* FindFreeBlock(size_t size, unsigned* found_first,
                                unsigned* found_second) {
  unsigned first;
  unsigned second;
  if (!MapSearchSize(size, &first, &second)) {
    return NULL;
  }
  heap.stats.bin_lookups++;

  unsigned int second_mask =
      heap.second_level_bitmaps[first] & (~0u << second);
  if (second_mask == 0) {
    size_t first_mask;
    if (first + 1 >= DAVE_HEAP_FL_COUNT) {
      return NULL;
    }
    first_mask = heap.first_level_bitmap & (~(size_t)0 << (first + 1));
    if (first_mask == 0) {
      return NULL;
    }
    first = FirstSetBitSize(first_mask);
    second_mask = heap.second_level_bitmaps[first];
  }
  second = FirstSetBitUnsigned(second_mask);
  *found_first = first;
  *found_second = second;
  return heap.bins[first][second];
}

static DaveBlock* CoalesceFreeBlock(DaveBlock* block) {
  DaveBlock* previous = PreviousPhysicalBlock(block);
  if (BlockIsFree(previous)) {
    RemoveFreeBlock(previous);
    SetBlock(previous, BlockSize(previous) + BlockSize(block), DAVE_BLOCK_FREE);
    block = previous;
  }

  DaveBlock* next = NextPhysicalBlock(block);
  if (BlockIsFree(next)) {
    RemoveFreeBlock(next);
    SetBlock(block, BlockSize(block) + BlockSize(next), DAVE_BLOCK_FREE);
  }
  NextPhysicalBlock(block)->previous_size = BlockSize(block);
  return block;
}

static int RegisterRegion(void* memory, size_t memory_size) {
  uintptr_t raw = (uintptr_t)memory;
  uintptr_t aligned = AlignPointer(raw);
  if (aligned < raw || memory_size < (size_t)(aligned - raw)) {
    return 0;
  }
  size_t size = AlignDown(memory_size - (size_t)(aligned - raw),
                          DAVE_HEAP_ALIGNMENT);
  size_t overhead =
      DAVE_REGION_HEADER_SIZE + 2 * DAVE_BLOCK_HEADER_SIZE;
  if (size < overhead + DAVE_MIN_BLOCK_SIZE) {
    return 0;
  }

  DaveRegion* region = (DaveRegion*)aligned;
  region->next = heap.regions;
  region->size = size;
  heap.regions = region;

  DaveBlock* start =
      (DaveBlock*)((char*)region + DAVE_REGION_HEADER_SIZE);
  SetBlock(start, DAVE_BLOCK_HEADER_SIZE, DAVE_BLOCK_SENTINEL);
  start->previous_size = 0;

  DaveBlock* free_block =
      (DaveBlock*)((char*)start + DAVE_BLOCK_HEADER_SIZE);
  size_t free_size = size - overhead;
  SetBlock(free_block, free_size, DAVE_BLOCK_FREE);
  free_block->previous_size = DAVE_BLOCK_HEADER_SIZE;

  DaveBlock* finish = (DaveBlock*)((char*)free_block + free_size);
  SetBlock(finish, DAVE_BLOCK_HEADER_SIZE, DAVE_BLOCK_SENTINEL);
  finish->previous_size = free_size;
  InsertFreeBlock(free_block);

  heap.stats.mapped_bytes += size;
  heap.stats.region_count++;
  return 1;
}

static size_t RegionSizeForRequest(size_t minimum_block_size) {
  size_t overhead =
      DAVE_REGION_HEADER_SIZE + 2 * DAVE_BLOCK_HEADER_SIZE;
  size_t minimum;
  if (minimum_block_size > (size_t)-1 - overhead) {
    return 0;
  }
  minimum = minimum_block_size + overhead;
  size_t configured = __initial_heap_size > 0
                          ? (size_t)__initial_heap_size
                          : (size_t)DAVE_HEAP_DEFAULT_SIZE;
  if (configured < minimum) {
    configured = minimum;
  }
#if defined(__DAVECC_NATIVE_LINUX__)
  if (!AlignUpChecked(configured, DAVE_HEAP_PAGE_SIZE, &configured)) {
    return 0;
  }
#else
  if (!AlignUpChecked(configured, DAVE_HEAP_ALIGNMENT, &configured)) {
    return 0;
  }
#endif
  return configured;
}

static int ExpandHeap(size_t minimum_block_size) {
#if defined(__DAVECC_NATIVE_LINUX__)
  size_t size = RegionSizeForRequest(minimum_block_size);
  if (size == 0) {
    return 0;
  }
  long mapping =
      syscall(DAVE_HEAP_MMAP, 0, size, 3, 0x22, -1, 0);
  if (mapping == -1) {
    return 0;
  }
  if (!RegisterRegion((void*)(uintptr_t)mapping, size)) {
    syscall(SYS_munmap, mapping, size, 0, 0, 0, 0);
    return 0;
  }
  return 1;
#elif !defined(__DAVECC__)
  size_t size = RegionSizeForRequest(minimum_block_size);
  if (size == 0) {
    return 0;
  }
  void* mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) {
    return 0;
  }
  return RegisterRegion(mapping, size);
#else
  (void)minimum_block_size;
  return 0;
#endif
}

static int InitializeHeap(void) {
  if (heap.initialized) {
    return 1;
  }
  heap.initialized = 1;

#if defined(__DAVECC_NATIVE_LINUX__)
  return ExpandHeap(DAVE_MIN_BLOCK_SIZE);
#elif defined(__DAVECC_HAS_HEAP_LOCK__)
  extern unsigned long long
      __davecc_guest_heap_storage[((1024 * 1024) + 15) /
                                  sizeof(unsigned long long)];
  return RegisterRegion(__davecc_guest_heap_storage,
                        sizeof(__davecc_guest_heap_storage));
#elif defined(__wasm32__)
  // Linear memory says nothing about its own shape, so the linker marks out
  // what is left above the shadow stack and the heap is exactly that.
  extern char __heap_base[];
  extern char __heap_end[];
  return RegisterRegion(__heap_base, (size_t)(__heap_end - __heap_base));
#elif defined(__x86_64__) || defined(__aarch64__) || defined(__arm__) || \
    defined(__risc_v__) || defined(__p_code__)
  extern char _end[];
  size_t size = __initial_heap_size > 0
                    ? (size_t)__initial_heap_size
                    : (size_t)DAVE_HEAP_DEFAULT_SIZE;
  return RegisterRegion(_end, size);
#else
  return ExpandHeap(DAVE_MIN_BLOCK_SIZE);
#endif
}

static int RequestBlockSize(size_t request, size_t* block_size) {
  if (request > (size_t)-1 - DAVE_BLOCK_HEADER_SIZE) {
    return 0;
  }
  size_t size;
  if (!AlignUpChecked(request + DAVE_BLOCK_HEADER_SIZE,
                      DAVE_HEAP_ALIGNMENT, &size)) {
    return 0;
  }
  if (size < DAVE_MIN_BLOCK_SIZE) {
    size = DAVE_MIN_BLOCK_SIZE;
  }
  *block_size = size;
  return 1;
}

static void AccountAllocation(size_t size) {
  heap.stats.allocated_bytes += size;
  if (heap.stats.allocated_bytes > heap.stats.peak_allocated_bytes) {
    heap.stats.peak_allocated_bytes = heap.stats.allocated_bytes;
  }
  heap.stats.allocation_count++;
}

static DaveBlock* AllocateFromFreeBlock(DaveBlock* block, size_t size,
                                        unsigned first, unsigned second) {
  size_t original_size = BlockSize(block);
  RemoveFreeBlockFromBin(block, first, second);
  size_t remaining = original_size - size;
  if (remaining >= DAVE_MIN_BLOCK_SIZE) {
    SetBlock(block, size, 0);
    DaveBlock* remainder = (DaveBlock*)((char*)block + size);
    SetBlock(remainder, remaining, DAVE_BLOCK_FREE);
    remainder->previous_size = size;
    NextPhysicalBlock(remainder)->previous_size = remaining;
    InsertFreeBlock(remainder);
  } else {
    size = original_size;
    SetBlock(block, size, 0);
    NextPhysicalBlock(block)->previous_size = size;
  }
  AccountAllocation(size);
  InitializeUserMarker(block);
  return block;
}

static DaveBlock* DirectMapBlock(size_t block_size) {
#if defined(__DAVECC_NATIVE_LINUX__)
  size_t mapping_size;
  if (!AlignUpChecked(block_size, DAVE_HEAP_PAGE_SIZE, &mapping_size)) {
    return NULL;
  }
  long mapping =
      syscall(DAVE_HEAP_MMAP, 0, mapping_size, 3, 0x22, -1, 0);
  if (mapping == -1) {
    return NULL;
  }
  DaveBlock* block = (DaveBlock*)(uintptr_t)mapping;
  SetBlock(block, mapping_size, DAVE_BLOCK_DIRECT);
  block->previous_size = 0;
  InitializeUserMarker(block);
  heap.stats.mapped_bytes += mapping_size;
  heap.stats.direct_mapped_bytes += mapping_size;
  AccountAllocation(mapping_size);
  return block;
#else
  (void)block_size;
  return NULL;
#endif
}

static void* AllocateGeneral(size_t request) {
  size_t block_size;
  if (!RequestBlockSize(request, &block_size)) {
    return NULL;
  }
  if (!InitializeHeap()) {
    return NULL;
  }

#if defined(__DAVECC_NATIVE_LINUX__)
  if (request >= DAVE_HEAP_LARGE_THRESHOLD) {
    DaveBlock* direct = DirectMapBlock(block_size);
    return direct == NULL ? NULL : BlockUserPointer(direct);
  }
#endif

  for (;;) {
    unsigned first;
    unsigned second;
    DaveBlock* block = FindFreeBlock(block_size, &first, &second);
    if (block != NULL) {
      return BlockUserPointer(
          AllocateFromFreeBlock(block, block_size, first, second));
    }
    if (!ExpandHeap(block_size)) {
      return NULL;
    }
  }
}

void* Malloc(size_t request) {
  if (!InitializeHeap()) {
    return NULL;
  }
  return AllocateGeneral(request);
}

static void* ResolveAlignedPointer(void* pointer) {
  size_t marker = ((size_t*)pointer)[-1];
  if (marker == DAVE_ALIGNED_MARKER) {
    return (void*)(uintptr_t)((size_t*)pointer)[-2];
  }
  return pointer;
}

size_t __davecc_malloc_cache_class(void* pointer) {
  if (pointer == NULL) {
    return 0;
  }
  void* resolved = ResolveAlignedPointer(pointer);
  if (resolved != pointer) {
    return 0;
  }
  DaveBlock* block = UserPointerBlock(resolved);
  if (BlockIsDirect(block)) {
    return 0;
  }
  size_t capacity = BlockSize(block) - DAVE_BLOCK_HEADER_SIZE;
  return capacity <= 1024 && (capacity & 15) == 0 ? capacity : 0;
}

void Free(void* pointer) {
  if (pointer == NULL) {
    return;
  }
  pointer = ResolveAlignedPointer(pointer);
  DaveBlock* block = UserPointerBlock(pointer);
  size_t size = BlockSize(block);

  if (BlockIsDirect(block)) {
#if defined(__DAVECC_NATIVE_LINUX__)
    heap.stats.allocated_bytes -= size;
    heap.stats.mapped_bytes -= size;
    heap.stats.direct_mapped_bytes -= size;
    heap.stats.free_count++;
    syscall(SYS_munmap, block, size, 0, 0, 0, 0);
#endif
    return;
  }

  heap.stats.allocated_bytes -= size;
  heap.stats.free_count++;
  SetBlock(block, size, DAVE_BLOCK_FREE);
  block = CoalesceFreeBlock(block);
  InsertFreeBlock(block);
}

static void SplitAllocatedBlock(DaveBlock* block, size_t requested_size) {
  size_t original_size = BlockSize(block);
  size_t remaining = original_size - requested_size;
  if (remaining < DAVE_MIN_BLOCK_SIZE) {
    return;
  }
  SetBlock(block, requested_size, 0);
  DaveBlock* remainder =
      (DaveBlock*)((char*)block + requested_size);
  SetBlock(remainder, remaining, DAVE_BLOCK_FREE);
  remainder->previous_size = requested_size;
  NextPhysicalBlock(remainder)->previous_size = remaining;
  remainder = CoalesceFreeBlock(remainder);
  InsertFreeBlock(remainder);
  heap.stats.allocated_bytes -= remaining;
}

static void* ReallocateByCopy(void* pointer, size_t old_capacity,
                              size_t request) {
  void* replacement = Malloc(request);
  if (replacement == NULL) {
    return NULL;
  }
  size_t copy_size = old_capacity < request ? old_capacity : request;
  memcpy(replacement, pointer, copy_size);
  Free(pointer);
  return replacement;
}

void* Realloc(void* pointer, size_t request) {
  if (pointer == NULL) {
    return Malloc(request);
  }
  if (request == 0) {
    Free(pointer);
    return NULL;
  }

  void* resolved = ResolveAlignedPointer(pointer);
  DaveBlock* block = UserPointerBlock(resolved);
  size_t old_size = BlockSize(block);
  size_t old_capacity = old_size - DAVE_BLOCK_HEADER_SIZE;
  size_t user_offset = (size_t)((char*)pointer - (char*)resolved);
  if (user_offset > old_capacity) {
    return NULL;
  }
  old_capacity -= user_offset;

  size_t requested_size;
  if (!RequestBlockSize(request, &requested_size)) {
    return NULL;
  }

  if (resolved != pointer || BlockIsDirect(block)) {
    if (resolved == pointer && BlockIsDirect(block) &&
        request >= DAVE_HEAP_LARGE_THRESHOLD && requested_size <= old_size) {
      return pointer;
    }
    return ReallocateByCopy(pointer, old_capacity, request);
  }

  if (requested_size <= old_size) {
    SplitAllocatedBlock(block, requested_size);
    return pointer;
  }

  DaveBlock* next = NextPhysicalBlock(block);
  if (BlockIsFree(next) &&
      old_size + BlockSize(next) >= requested_size) {
    RemoveFreeBlock(next);
    size_t combined = old_size + BlockSize(next);
    SetBlock(block, combined, 0);
    NextPhysicalBlock(block)->previous_size = combined;
    heap.stats.allocated_bytes += combined - old_size;
    SplitAllocatedBlock(block, requested_size);
    return BlockUserPointer(block);
  }

  return ReallocateByCopy(pointer, old_capacity, request);
}

static int BlockAppearsInBin(DaveBlock* wanted) {
  unsigned first;
  unsigned second;
  MapBlockSize(BlockSize(wanted), &first, &second);
  DaveBlock* block = heap.bins[first][second];
  size_t maximum = heap.stats.mapped_bytes / DAVE_MIN_BLOCK_SIZE + 1;
  size_t count = 0;
  while (block != NULL && count++ < maximum) {
    if (block == wanted) {
      return 1;
    }
    block = FreeLinks(block)->next;
  }
  return 0;
}

int __davecc_heap_check(void) {
  if (!heap.initialized) {
    return 1;
  }
  DaveRegion* region = heap.regions;
  while (region != NULL) {
    DaveBlock* block =
        (DaveBlock*)((char*)region + DAVE_REGION_HEADER_SIZE);
    if (!BlockIsSentinel(block) ||
        BlockSize(block) != DAVE_BLOCK_HEADER_SIZE) {
      return 0;
    }
    size_t walked = DAVE_REGION_HEADER_SIZE;
    size_t previous_size = 0;
    for (;;) {
      size_t size = BlockSize(block);
      if (size < DAVE_BLOCK_HEADER_SIZE ||
          (size & (DAVE_HEAP_ALIGNMENT - 1)) != 0 ||
          block->previous_size != previous_size) {
        return 0;
      }
      walked += size;
      if (BlockIsSentinel(block) && previous_size != 0) {
        break;
      }
      if (BlockIsFree(block) && !BlockAppearsInBin(block)) {
        return 0;
      }
      previous_size = size;
      block = (DaveBlock*)((char*)block + size);
      if (walked > region->size) {
        return 0;
      }
    }
    if (walked != region->size) {
      return 0;
    }
    region = region->next;
  }
  return 1;
}

void __davecc_heap_get_stats(DaveHeapStats* stats) {
  if (stats == NULL) {
    return;
  }
  *stats = heap.stats;
  stats->free_bytes = 0;
  stats->largest_free_block = 0;
  for (unsigned first = 0; first < DAVE_HEAP_FL_COUNT; ++first) {
    for (unsigned second = 0; second < DAVE_HEAP_SL_COUNT; ++second) {
      DaveBlock* block = heap.bins[first][second];
      while (block != NULL) {
        size_t size = BlockSize(block);
        stats->free_bytes += size;
        if (size > stats->largest_free_block) {
          stats->largest_free_block = size;
        }
        block = FreeLinks(block)->next;
      }
    }
  }
}

void PrintFreeList(const char* tag) {
  DaveHeapStats stats;
  __davecc_heap_get_stats(&stats);
  printf("%s: regions=%zu mapped=%zu allocated=%zu free=%zu largest=%zu\n",
         tag, stats.region_count, stats.mapped_bytes, stats.allocated_bytes,
         stats.free_bytes, stats.largest_free_block);
}

#endif
