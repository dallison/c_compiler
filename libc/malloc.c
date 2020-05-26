//
//  malloc.c
//  c_compiler
//
//  Created by David Allison on 1/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

void* calloc(size_t n, size_t m) {
  void* p = malloc(n*m);
  if (p == NULL) {
    return NULL;
  }
  memset(p, 0, n*m);
  return p;
}
