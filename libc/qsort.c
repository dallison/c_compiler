//
//  qsort.c
//  c_compiler
//
//  Created by David Allison on 6/29/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>

static void Swap(void* p, void* q, size_t size) {
  char* a = p;
  char* b = q;

  for (size_t i = 0; i < size; i++) {
    char tmp = *a;
    *a = *b;
    *b = tmp;
    a++;
    b++;
  }
}

// Quicksort base using comparison function.
// Args:
// size: size of each element
// first: byte offset from base for first value
// last: byte offset from base for last value
// Sorts first...last inclusive.
static void Quicksort(void *base, size_t size, ssize_t first,
                      ssize_t last,
           int (*compar)(const void *, const void *)) {
  if (first < last) {
    size_t x = (first + last) / 2;    // Choose pivot as middle of range.
    x -= x % size;                    // Aligned down to size.
    ssize_t i = first;
    ssize_t j = last;
  
    do {
      // Move i up to next greater than or equal to pivot.
      while (compar(base+i, base+x) < 0) {
        i += size;
      }
      // Move j down to next less than or equal to pivot.
      while (compar(base+j, base+x) > 0) {
        j -= size;
      }
      if (i <= j) {
        // Swap base[i] and base[j];
        if (i != j) {
          // If we move the pivot, keep track of where it went.
          if (x == i) {
            x = j;
          } else if (x == j) {
            x = i;
          }
          Swap(base+i, base+j, size);
        }
        i += size;
        j -= size;
      }
    } while (i <= j);
    // We have partitioned the array into first..j and i..last.  Everything
    // in first..j is less than or equal to the pivot and all in i...last is
    // greater than or equal to it,
    Quicksort(base, size, first, j, compar);
    Quicksort(base, size, i, last, compar);
  }
}
  
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  Quicksort(base, size, 0, nmemb * size - size, compar);
}

