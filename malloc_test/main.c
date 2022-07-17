//
//  main.c
//  malloc_test
//
//  Created by David Allison on 10/14/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

extern void* Malloc(size_t n);
extern void Free(void* p);
extern void PrintFreeList(const char* tag);
extern void* Realloc(void* p, size_t n);

int main(int argc, const char * argv[]) {
  void* p1 = Malloc(100);
  PrintFreeList("after p1");
  void* p2 = Malloc(200);
  PrintFreeList("after p2");
  void* p3 = Malloc(300);
  PrintFreeList("after p3");
  Free(p2);
  PrintFreeList("after free p2");
  Free(p1);
  PrintFreeList("after free p1");
  Free(p3);
  PrintFreeList("after free p3");
  
  p1 = Malloc(100);
  PrintFreeList("after p1");
  p2 = Malloc(200);
  PrintFreeList("after p2");
  p3 = Malloc(300);
  Free(p2);
  PrintFreeList("after free p2");
  p1 = Realloc(p1, 160);
  PrintFreeList("after realloc p1");
  p2 = Realloc(p2, 400);
  PrintFreeList("after realloc p2");
}
