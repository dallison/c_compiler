//
//  realloc.c
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__6502__) || defined(__risc_v__) || defined(__aarch64__) || \
    defined(__arm__) || defined(__p_code__)
#include "6502/_malloc.h"

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc
#elif defined(__x86_64__)
#include "6502/_malloc.h"
extern void* Malloc(size_t n);
extern void Free(void* p);
#else
#include "_malloc.h"
extern void* Malloc(size_t n);
extern void Free(void* p);
#endif

static size_t AlignSize(size_t s) {
  return (s + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1);
}

static void ShrinkBlock(FreeBlockHeader* alloc_block, size_t orig_length,
                        size_t new_length, size_t* len_ptr) {
  assert(new_length < orig_length);
  size_t rem = orig_length - new_length;
  if (rem >= sizeof(FreeBlockHeader)) {
    // If we are freeing enough to make a free block, free it, otherwise
    // there's nothing we can do and we just keep the block the same size.
    *len_ptr = new_length;  // Change size of block.
    size_t* newp = (size_t*)((char*)alloc_block + sizeof(size_t) + new_length);
    *newp = rem - sizeof(size_t);  // Add header for free.
    Free(newp + 1);
  }
}

static void ExpandIntoFreeBlockAbove(FreeBlockHeader* free_block,
                                     size_t new_length, size_t len_diff,
                                     size_t free_remaining, size_t* len_ptr,
                                     FreeBlockHeader** next_ptr) {
  assert(free_remaining > sizeof(FreeBlockHeader));
  FreeBlockHeader* next = free_block->next;

  // The free block has enough space.
  *len_ptr = new_length;
  FreeBlockHeader* new_block =
      (FreeBlockHeader*)((uintptr_t)free_block + len_diff);
  new_block->length = free_remaining;
  new_block->next = next;
  *next_ptr = new_block;
}

static size_t* MergeWithFreeBlockBelow(void* alloc_block, FreeBlockHeader* prev,
                                       FreeBlockHeader* free_block,
                                       size_t new_length, size_t orig_length) {
  uintptr_t free_addr = (uintptr_t)free_block;

  FreeBlockHeader** next_ptr;
  if (prev == NULL) {
    next_ptr = &__free_list;
  } else {
    next_ptr = &prev->next;
  }
  // Move FreeBlockHeader to end of allocated block.  This is inside
  // the combined free block and block being reallocated.
  FreeBlockHeader* next = free_block->next;
  FreeBlockHeader* newb =
      (FreeBlockHeader*)(free_addr + new_length + sizeof(size_t));
  newb->length = free_block->length + orig_length - new_length;
  newb->next = next;
  *next_ptr = newb;

  size_t* len_ptr = (size_t*)free_block;
  *len_ptr = new_length;
  memmove(len_ptr + 1, alloc_block, orig_length);
  return len_ptr + 1;
}

void* Realloc(void* p, size_t n) {
  if (p == NULL) {
    // No block to realloc, just call malloc.
    return Malloc(n);
  }
  // The allocated block has its length immediately prior to its address.
  size_t* len_ptr = (size_t*)p - 1;
  size_t orig_length = *len_ptr;
  FreeBlockHeader* alloc_block =
      (FreeBlockHeader*)((uintptr_t)p - sizeof(size_t));
  uintptr_t alloc_addr = (uintptr_t)p;

  n = AlignSize(n);  // Aligned.
  if (n == orig_length) {
    // Same size as current block, nothing to do.
    return p;
  }
  if (n < orig_length) {
    // Decreasing in size.  Free the remaining part.
    ShrinkBlock(alloc_block, orig_length, n, len_ptr);
    return p;
  }

  // Increasing in size.
  // See if there's a free block immediately following allocated block.
  FreeBlockHeader* free_block = __free_list;
  FreeBlockHeader* prev = NULL;
  FreeBlockHeader* prev_prev = NULL;
  while (free_block != NULL) {
    FreeBlockHeader** next_ptr;
    if (prev == NULL) {
      next_ptr = &__free_list;
    } else {
      next_ptr = &prev->next;
    }
    if (free_block > alloc_block) {
      uintptr_t free_addr = (uintptr_t)free_block;
      size_t diff = n - orig_length;
      if (alloc_addr + orig_length == free_addr) {
        // There is a free block above.  See if has enough space.
        if (free_block->length > diff) {
          ssize_t freelen = free_block->length - diff;
          if (freelen > sizeof(FreeBlockHeader)) {
            ExpandIntoFreeBlockAbove(free_block, n, diff, freelen, len_ptr,
                                     next_ptr);
            return p;
          }
        }
      }
      // Check for free block adjacent below.
      if (prev != NULL) {
        uintptr_t prev_addr = (uintptr_t)prev;
        if (prev_addr + prev->length == (uintptr_t)alloc_block &&
            prev->length >= diff) {
          // Previous free block is adjacent and has enough space in it.
          // Use start of new block as new address and place FreeBlockHeader
          // at newly free part.
          return MergeWithFreeBlockBelow(p, prev_prev, prev, n, orig_length);
        }
        // Block doesn't have enough space.
        break;
      }
    }
    prev_prev = prev;
    prev = free_block;
    free_block = free_block->next;
  }

  // If we get here we can't reuse the existing block.  We allocate a new
  // one, copy the memory and free the old block.  We are guaranteed that
  // the new block is larger than the original one since if it was smaller
  // we can always reuse the block.
  void* newp = Malloc(n);
  if (newp == NULL) {
    return NULL;
  }
  memcpy(newp, p, orig_length);
  Free(p);
  return newp;
}
