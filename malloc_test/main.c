//
//  main.c
//  malloc_test
//
//  Created by David Allison on 10/14/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

extern void* Malloc(size_t n);
extern void Free(void* p);
extern void PrintFreeList(const char* tag);
extern void* Realloc(void* p, size_t n);

extern int __initial_heap_size;

int main(int argc, const char * argv[]) {
  // Heap is 1024 bytes.
  __initial_heap_size = 2048;
  
  void* p1 = Malloc(100);
  PrintFreeList("after p1");
  void* p2 = Malloc(200);
  PrintFreeList("after p2");
  void* p3 = Malloc(300);
  PrintFreeList("after p3");
  void* p4 = Malloc(384);
  PrintFreeList("after p4");
  Free(p2);
  PrintFreeList("after free p2");
  Free(p1);
  PrintFreeList("after free p1");
  Free(p3);
  PrintFreeList("after free p3");
  Free(p4);
  PrintFreeList("after free p4");
  // All free, should have a single block in the free list.
  
  p1 = Malloc(200);
  PrintFreeList("after p1");
  p2 = Malloc(100);
  PrintFreeList("after p2");
  p3 = Malloc(300);
  PrintFreeList("after p3");
  Free(p2);
  p2 = NULL;
  
  PrintFreeList("after free p2");
  p4 = Malloc(210);
  PrintFreeList("after p4");
  void* p5 = Malloc(120);
  Free(p4);
  p4 = NULL;
  PrintFreeList("after p5");
  void* p6 = Malloc(300);
  PrintFreeList("after p6");

  // Three blocks in free list.  p2 and p4 are free and invalid.
  
  // Realloc with free block adjacent below.
  p5 = Realloc(p5, 128);
  PrintFreeList("after realloc p5");

  // Shrink p1 from 200 to 160.
  p1 = Realloc(p1, 160);
  PrintFreeList("after realloc p1");
  
  // Realloc to block adjacent above.
  p6 = Realloc(p6, 400);
  PrintFreeList("after realloc p5");
  
  // p1 moves in memory
  p1 = Realloc(p1, 600);
  PrintFreeList("after realloc p1");

  Free(p1);
  Free(p3);
  PrintFreeList("after free p3");
  p1 = Malloc(100);
  PrintFreeList("after p1");
  p2 = Malloc(20);
  
  Free(p1);
  Free(p2);
  Free(p5);
  Free(p6);
  PrintFreeList("after free all");
  
  p1 = Malloc(1);
  Free(p1);
  
}
