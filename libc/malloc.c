//
//  malloc.c
//  c_compiler
//
//  Created by David Allison on 1/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if !defined(__6502__) && !defined(__risc_v__)
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#endif

#if 0
#define STATIC static
#else
#define STATIC
#endif

#if defined(__6502__) || defined(__risc_v__)
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
#include "6502/_malloc.h"
#endif

#if defined(__6502__)
#define MEMTOP 0xc000
#endif

FreeBlockHeader* __free_list;

STATIC void InitFreeList() {
#if defined(__6502__)
  __free_list = (FreeBlockHeader*)_end;
  __free_list->length = MEMTOP - (int)_end;
#elif defined(__risc_v__)
  // TODO
#else
  // Not on 6502, call mmap to get memory.
  const int kHeapSize = 1024*1024;
  __free_list = mmap(0, kHeapSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (__free_list == MAP_FAILED) {
    printf("errno: %d\n", errno);
    abort();
  }
  __free_list->length = kHeapSize;
#endif
  __free_list->next = NULL;
}

#if !defined(__6502__) && !defined(__risc_v__)
void PrintFreeList(const char* tag) {
  FreeBlockHeader* b = __free_list;
  while (b != NULL) {
    printf("%s: block %p: length: %zd, next: %p\n", tag, b, b->length, b->next);
    b = b->next;
  }
}
#endif

STATIC size_t AlignSize(size_t s) {
  return (s + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1);
}

void* Malloc(size_t n) {
  if (__free_list == NULL) {
    InitFreeList();
  }
  n = AlignSize(n);      // Aligned.
  size_t full_length = n + sizeof(size_t);
  FreeBlockHeader* b = __free_list;
  FreeBlockHeader* prev = NULL;
  while (b != NULL) {
    if (b->length >= full_length) {
      // Free block is big enough.  Move
      size_t rem = b->length - full_length;
      if (rem >= sizeof(FreeBlockHeader)) {
        FreeBlockHeader* next = (FreeBlockHeader*)((uintptr_t)b + full_length);
        next->length = rem;
         // Remove from free list.
        if (prev == NULL) {
          __free_list = next;
        } else {
          prev->next = next;
        }
        size_t* newblock = (size_t*)b;      // Start of new block.
        *newblock = n;     // Size of allocated block.
        return (void*)((uintptr_t)b + sizeof(size_t));    // Address of new block.
      }
    }
    prev = b;
    b = b->next;
  }
  return NULL;
}




