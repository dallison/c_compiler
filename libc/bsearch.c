//
//  bsearch.c
//  c_compiler
//
//  Created by David Allison on 6/29/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>

// The array is arranged as a sequence of nmemb members each with size
// size.  The arg base is the base address of the array.  The arg key
// is the value to search for.  The compar arg is a function to call to
// do the comparison (strcmp semantics).
void *bsearch(const void *key, const void *base,
              size_t nmemb, size_t size,
              int (*compar)(const void *
                            , const void *)) {
  size_t low = 0;
  size_t len = nmemb * size;
  size_t high = len;              // One beyond end of array.
  while (low < high) {
    size_t mid = low + (high - low) / 2;   // Offset to middle.
    mid -= mid % size;                     // Aligned down to size.
    int v = compar(key, base+mid);
    if (v == 0) {
      // Found.
      return (void*)base + mid;
    }
    if (v < 0) {
      high = mid;
    } else {
      low = mid + size;
    }
  }
  return NULL;
}
