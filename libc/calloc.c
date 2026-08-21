//
//  calloc.c
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#if defined(__6502__) || defined(__p_code__)

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc

#elif !defined(__DAVECC_HAS_HEAP_LOCK__)
extern void* Malloc(size_t n);
extern void Free(void* p);

#endif

#if defined(__DAVECC_HAS_HEAP_LOCK__)
void* calloc(size_t n, size_t m) {
  if (m != 0 && n > (size_t)-1 / m) {
    return NULL;
  }
  size_t size = n * m;
  void* p = malloc(size);
  if (p == NULL) {
    return NULL;
  }
  memset(p, 0, size);
  return p;
}
#else
void* Calloc(size_t n, size_t m) {
  if (m != 0 && n > (size_t)-1 / m) {
    return NULL;
  }
  size_t size = n * m;
  void* p = Malloc(size);
  if (p == NULL) {
    return NULL;
  }
  memset(p, 0, size);
  return p;
}
#endif
