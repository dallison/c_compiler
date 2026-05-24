//
//  malloc.c
//  c_compiler
//
//  Created by David Allison on 1/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if !defined(__6502__) && !defined(__risc_v__)
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#if 0
#define STATIC static
#else
#define STATIC
#endif

#if defined(__6502__) || defined(__risc_v__) || defined(__x86_64__)
#include "6502/_malloc.h"

// Defined by linker at end of .bss section.  This is the start
// of the memory available for the heap.
extern char _end[];

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc

#else
#include "_malloc.h"
#endif

#if defined(__6502__)
#define MEMTOP 0xc000
#endif

FreeBlockHeader* __free_list;
int __initial_heap_size;

STATIC void InitFreeList() {
#if defined(__6502__)
  __free_list = (FreeBlockHeader*)_end;
  __free_list->length = MEMTOP - (int)_end;
#elif defined(__risc_v__)
// TODO
#elif defined(__x86_64__)
  extern char _end[];
  __free_list = (FreeBlockHeader*)_end;
  if (__initial_heap_size == 0) {
    __initial_heap_size = 1024 * 1024;
  }
  __free_list->length = __initial_heap_size;
#else
  // Not on 6502, call mmap to get memory.
  __free_list = mmap(0, __initial_heap_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (__free_list == MAP_FAILED) {
    printf("errno: %d\n", errno);
    abort();
  }
  __free_list->length = __initial_heap_size;
#endif
  __free_list->next = NULL;
}

#if !defined(__6502__) && !defined(__risc_v__)
void PrintFreeList(const char* tag) {
  FreeBlockHeader* b = __free_list;
  while (b != NULL) {
    printf("%s: block %p: length: %zd, end: %p next: %p\n", tag, b, b->length,
           (char*)b + b->length, b->next);
    b = b->next;
  }
}
#endif

STATIC size_t AlignSize(size_t s) {
#ifdef ALIGN
  return (s + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1);
#else
  return s;
#endif
}

// Expand the heap if we can.
STATIC FreeBlockHeader* ExpandHeap(void) { return NULL; }

STATIC size_t TakeStartOfFreeBlock(FreeBlockHeader* block, size_t num_bytes,
                                   size_t full_length, FreeBlockHeader* prev) {
  assert(block->length > full_length);
  
  size_t rem = block->length - full_length;
  if (rem >= sizeof(FreeBlockHeader)) {
    FreeBlockHeader* next = (FreeBlockHeader*)((uintptr_t)block + full_length);
    next->length = rem;
    next->next = block->next;
    // Remove from free list.
    if (prev == NULL) {
      // No previous free block, this becomes the first in the list.
      __free_list = next;
    } else {
      // Chain to previous free block.
      prev->next = next;
    }
  } else {
    // We have less than sizeof(FreeBlockHeader)
    // Take whole block.
    if (prev == NULL) {
      __free_list = block->next;
    } else {
      prev->next = block->next;
    }
    // Allocate whole block.
    num_bytes = block->length - sizeof(size_t);
  }
  return num_bytes;
}

void* Malloc(size_t n) {
  if (__free_list == NULL) {
    InitFreeList();
  }
  n = AlignSize(n);  // Aligned.
  size_t full_length = n + sizeof(size_t);
  FreeBlockHeader* free_block = __free_list;
  FreeBlockHeader* prev = NULL;
  while (free_block != NULL) {
    if (free_block->length >= full_length) {
      // Free block is big enough.  If there's enough room for the free block
      // header, take the lower part of the free block and keep the remainder
      // in the free list.
      n = TakeStartOfFreeBlock(free_block, n, full_length, prev);
      size_t* newblock = (size_t*)free_block;  // Start of new block.
      *newblock = n;                           // Size of allocated block.
      return (void*)((uintptr_t)free_block + sizeof(size_t));
    }
    prev = free_block;
    free_block = free_block->next;
    if (free_block == NULL) {
      // Expand the heap if we can.
      free_block = ExpandHeap();
    }
  }
  return NULL;
}
