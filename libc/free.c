//
//  free.c
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#if defined(__6502__) || defined(__risc_v__) || defined(__x86_64__) || defined(__aarch64__)
#include "6502/_malloc.h"

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc
#else
#include "_malloc.h"
#endif

static void MergeWithAboveIfPossible(FreeBlockHeader* alloc_block,
                                     FreeBlockHeader* alloc_header,
                                     FreeBlockHeader* free_block,
                                     FreeBlockHeader** next_ptr,
                                     size_t alloc_length) {
  uintptr_t alloc_addr = (uintptr_t)alloc_block;
  uintptr_t free_addr = (uintptr_t)free_block;

  if (alloc_addr + alloc_length == free_addr) {
    // Merge with block above.
    alloc_header->next = free_block->next;
    alloc_header->length =
        alloc_length + sizeof(FreeBlockHeader*) + free_block->length;
    *next_ptr = alloc_header;
  } else {
    // Not adjacent to above; add to free list.
    // t points to the allocated block header which has its length set.
    alloc_header->length += sizeof(FreeBlockHeader*);
    alloc_header->next = free_block;
    *next_ptr = alloc_header;
  }
}

static bool MergeWithBelowIfPossible(FreeBlockHeader* free_block,
                                     FreeBlockHeader* prev) {
  uintptr_t prev_addr = (uintptr_t)prev;
  if (prev_addr + prev->length == (uintptr_t)free_block) {
    // Lower block is adjacent.
    prev->next = free_block->next;
    prev->length += free_block->length;
    return true;
  }
  return false;
}

static void InsertNewFreeBlockAtEnd(FreeBlockHeader* free_block,
                                    FreeBlockHeader* prev, size_t length) {
  free_block->length = length;
  free_block->next = NULL;
  if (prev == NULL) {
    __free_list = free_block;
  } else {
    prev->next = free_block;
  }
}

void Free(void* p) {
  // An allocated block has its length immediately before its address.
  size_t alloc_length = *((size_t*)p - 1);  // Length of allocated block.

  // Point to real start of allocated block.
  FreeBlockHeader* alloc_header =
      (FreeBlockHeader*)((uintptr_t)p - sizeof(size_t));

  // Insert into free list by searching for the appropriate point in memory
  // sorted by address.
  FreeBlockHeader* free_block = __free_list;
  if (free_block == NULL) {
    // No free list, this block becomes the only block.
    alloc_header->length = alloc_length + sizeof(size_t);
    alloc_header->next = NULL;
    __free_list = alloc_header;
    return;
  }
  FreeBlockHeader* prev = NULL;
  while (free_block != NULL) {
    FreeBlockHeader** next_ptr;
    if (prev == NULL) {
      next_ptr = &__free_list;
    } else {
      next_ptr = &prev->next;
    }
    // If the current block (b) is at a higher address than t then we know
    // that we need to insert t before b.
    if (free_block > alloc_header) {
      // Found a free block after the one being freed.
      MergeWithAboveIfPossible(p, alloc_header, free_block, next_ptr,
                               alloc_length);

      // See if we can merge with prev.  If the block just freed is
      // immediately contiguous with the previous free block,
      // we can merge them,
      if (prev != NULL) {
        MergeWithBelowIfPossible(alloc_header, prev);
      }
      // We're done.
      return;
    }
    // Look at the next free block, keeping track of the previous.
    prev = free_block;
    free_block = free_block->next;
  }
  // We reached the end of the free list, insert free block at end.
  if (prev != NULL) {
    if (MergeWithBelowIfPossible(alloc_header, prev)) {
      return;
    }
  }
  // Can't merge, insert a new free block at end.
  InsertNewFreeBlockAtEnd(alloc_header, prev,
                          alloc_header->length + sizeof(FreeBlockHeader*));
}
