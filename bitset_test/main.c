//
//  main.c
//  bitset_test
//
//  Created by David Allison on 7/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include "bitset.h"

int main(int argc, const char * argv[]) {
  BitSet set;
  BitSetInit(&set);
  
  int values[] = {1, 3, 4, 31, 120, 32, 33, 60, 63, 68,97,120,-1};
  for (int i = 0; values[i] != -1; i++) {
    BitSetInsert(&set, values[i]);
  }
  BitSetIterator it;
  BitSetIteratorStart(&it, &set);
  while (!BitSetIteratorDone(&it)) {
    size_t v = BitSetIteratorValue(&it);
    printf("%zd\n", v);
    BitSetIteratorNext(&it);
  }
}
