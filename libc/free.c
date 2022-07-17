//
//  free.c
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#if defined(__6502__) || defined(__risc_v__)
#include "6502/_malloc.h"

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc
#else
#include "6502/_malloc.h"
#endif

void Free(void* p) {
  size_t plen = *((size_t*)p - 1);
  FreeBlockHeader* t = (FreeBlockHeader*)((uintptr_t)p - sizeof(size_t));

  // Insert into free list.
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
      // Found a free block after the one being freed.
      uintptr_t paddr = (uintptr_t)p;
      uintptr_t baddr = (uintptr_t)b;
      if (paddr + plen == baddr) {
        // Merge with block above.
        t->next = b->next;
        t->length = plen + sizeof(size_t) + b->length;
        *pr = t;
      } else {
        // Not adjacent to above; add to free list.
        // t points to the allocated block header which has its length set.
        t->length += sizeof(size_t);
        t->next = b;
        *pr = t;
      }
      // See if we can merge with prev.
      if (prev != NULL) {
        uintptr_t praddr = (uintptr_t)prev;
        if (praddr + prev->length == (uintptr_t)t) {
          // Lower block is adjacent.
          prev->next = t->next;
          prev->length += t->length;
        }
      }
      return;
    }
    prev = b;
    b = b->next;
  }
}

