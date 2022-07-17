//
//  realloc.c
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#if defined(__6502__)
#include "6502/_malloc.h"

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc

#else
#include "6502/_malloc.h"
extern void* Malloc(size_t n);
extern void Free(void* p);
#endif

static size_t AlignSize(size_t s) {
  return (s + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1);
}

void* Realloc(void* p, size_t n) {
  if (p == NULL) {
    return Malloc(n);
  }
  size_t* lenptr = (size_t*)p - 1;
  size_t plen = *lenptr;
  FreeBlockHeader* t = (FreeBlockHeader*)((uintptr_t)p - sizeof(size_t));
  uintptr_t paddr = (uintptr_t)p;
  
  n = AlignSize(n);      // Aligned.
  if (n == plen) {
    return p;
  }
  if (n < plen) {
    // Decreasing in size.  Free the remaining part.
    size_t rem = plen - n;
    *lenptr = n;          // Change size of block.
    if (rem >= sizeof(FreeBlockHeader)) {
      Free((char*)p + n);
    }
    return p;
  }
  
  // Increasing in size.
  // See if there's a free block immediately following allocated block.
  FreeBlockHeader* b = __free_list;
  FreeBlockHeader* prev = NULL;
  while (b != NULL) {
    FreeBlockHeader** pr;
    if (prev == NULL) {
       pr = &__free_list;
     } else {
       pr = &prev->next;
     }
    if (b > t) {
      uintptr_t baddr = (uintptr_t)b;
      if (paddr + plen == baddr) {
        // There is a free block above.  See if has enough space.
        size_t diff = n - plen;
        ssize_t freelen = b->length - diff;
        if (freelen > sizeof(FreeBlockHeader)) {
          // The free block has enough space.
          *lenptr = n;
          FreeBlockHeader* newb = (FreeBlockHeader*)((uintptr_t)b + diff);
          newb->length = diff;
          newb->next = b->next;
          *pr = newb;
          return p;
        }
        // Block doesn't have enough space.
        break;
       }
    }
    prev = b;
    b = b->next;
  }
  
  // If we get here we can't reuse the existing block.
  void* newp = Malloc(n);
  memcpy(newp, p, plen);
  Free(p);
  return newp;
}

