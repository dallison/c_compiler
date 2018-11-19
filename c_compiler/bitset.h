//
//  BitSet.h
//  c_compiler
//
//  Created by David Allison on 12/14/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef BitSet_h
#define BitSet_h

#include <stdbool.h>
#include <stdint.h>
#include "vector.h"

typedef struct {
  uint32_t* value;
  size_t capacity;  // Capacity in words, not bytes.
} BitSet;

void BitSetInit(BitSet* set);
BitSet* NewBitSet(void);
void BitSetDestruct(BitSet* set);
void BitSetDelete(BitSet* set);
void BitSetCopy(BitSet* to, BitSet* from);
size_t BitSetCount(BitSet* set);

void BitSetClear(BitSet* set);

void BitSetInsert(BitSet* set, size_t index);
bool BitSetContains(BitSet* set, size_t index);
void BitSetRemove(BitSet* set, size_t index);

void BitSetIntersection(BitSet* set1, BitSet* set2, BitSet* result);
void BitSetUnion(BitSet* set1, BitSet* set2, BitSet* result);

bool BitSetEqual(BitSet* set1, BitSet* set2);

// Expand the values in a bitset to a vector of ints.
void BitSetExpand(BitSet* set, Vector* vec);

void BitSetPrint(BitSet* set);

#endif /* BitSet_h */
