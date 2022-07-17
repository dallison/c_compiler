//
//  calloc.c
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__6502__)

// Rename functions to libc names.
#define Malloc malloc
#define Free free
#define Calloc calloc
#define Realloc realloc

#else
extern void* Malloc(size_t n);
extern void Free(void* p);

#endif

void* Calloc(size_t n, size_t m) {
  void* p = Malloc(n*m);
  if (p == NULL) {
    return NULL;
  }
  memset(p, 0, n*m);
  return p;
}
