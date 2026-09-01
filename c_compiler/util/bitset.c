//
//  BitSet.c
//  c_compiler
//
//  Created by David Allison on 12/14/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "bitset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void BitSetInit(BitSet* set) {
  set->value = NULL;
  set->capacity = 0;
}

BitSet* NewBitSet() {
  BitSet* set = malloc(sizeof(BitSet));
  BitSetInit(set);
  return set;
}

void BitSetDestruct(BitSet* set) {
  free(set->value);
  set->value = NULL;
  set->capacity = 0;
}

void BitSetDelete(BitSet* set) {
  BitSetDestruct(set);
  free(set);
}

void BitSetClear(BitSet* set) {
  if (set->value == NULL) {
    return;
  }
  memset(set->value, 0, set->capacity * sizeof(uint64_t));
}

static int FindFirstSet64(uint64_t value) {
  if (value == 0) {
    return 0;
  }
  return __builtin_ctzll(value) + 1;
}

// Make room for an index into the set.
static void MakeRoomFor(BitSet* set, size_t index) {
  size_t words_required = (index + 64) / 64;
  if (set->value == NULL) {
    set->value = calloc(words_required, sizeof(uint64_t));
    set->capacity = words_required;
  } else if (words_required > set->capacity) {
    set->value = realloc(set->value, words_required * sizeof(uint64_t));
    // Clear new memory.
    memset(&set->value[set->capacity], 0,
           (words_required - set->capacity) * sizeof(uint64_t));
    set->capacity = words_required;
  }
}

// Make room for a number of words.
static void MakeRoom(BitSet* set, size_t words) {
  if (set->value == NULL) {
    set->value = calloc(words, sizeof(uint64_t));
    set->capacity = words;
  } else if (words > set->capacity) {
    set->value = realloc(set->value, words * sizeof(uint64_t));
    set->capacity = words;
  }
}

void BitSetFill(BitSet* set, size_t bit_count) {
  size_t words = (bit_count + 63) / 64;
  if (words != 0) {
    MakeRoom(set, words);
    memset(set->value, 0xff, words * sizeof(uint64_t));
    size_t final_word_bits = bit_count % 64;
    if (final_word_bits != 0) {
      set->value[words - 1] =
          (UINT64_C(1) << final_word_bits) - UINT64_C(1);
    }
  }
  if (set->capacity > words) {
    memset(&set->value[words], 0,
           (set->capacity - words) * sizeof(uint64_t));
  }
}

void BitSetInsert(BitSet* set, size_t index) {
  size_t word = index / 64;
  if (word >= set->capacity) {
    MakeRoomFor(set, index);
  }
  set->value[word] |= UINT64_C(1) << (index % 64);
}

bool BitSetContains(BitSet* set, size_t index) {
  size_t word = index / 64;
  if (word >= set->capacity) {
    return false;
  }
  return (set->value[word] & (UINT64_C(1) << (index % 64))) != 0;
}

size_t BitSetFindFirstSet(BitSet* set) {
  for (size_t word = 0; word < set->capacity; word++) {
    uint64_t w = set->value[word];
    int index = FindFirstSet64(w);
    if (index != 0) {
      return word * 64 + index - 1;
    }
  }
  return (size_t)-1;
}

size_t BitSetFindFirstClear(BitSet* set) {
  for (size_t word = 0; word < set->capacity; word++) {
    uint64_t w = ~set->value[word];
    int index = FindFirstSet64(w);
    if (index != 0) {
      return word * 64 + index - 1;
    }
  }
  return (size_t)-1;
}

void BitSetRemove(BitSet* set, size_t index) {
  size_t word = index / 64;
  if (word >= set->capacity) {
    MakeRoomFor(set, index);
  }
  set->value[word] &= ~(UINT64_C(1) << (index % 64));
}

void BitSetIntersection(BitSet* set1, BitSet* set2, BitSet* result) {
  size_t min = set1->capacity;
  if (min > set2->capacity) {
    min = set2->capacity;
  }
  // If the min size is zero then the result is an empty set.
  if (min == 0) {
    return;
  }
  MakeRoom(result, min);
  memcpy(result->value, set1->value, min * sizeof(uint64_t));
  for (size_t i = 0; i < min && i < set2->capacity; i++) {
    result->value[i] &= set2->value[i];
  }
}

void BitSetUnion(BitSet* set1, BitSet* set2, BitSet* result) {
  size_t max = set1->capacity;
  if (max < set2->capacity) {
    max = set2->capacity;
  }
  MakeRoom(result, max);
  memcpy(result->value, set1->value, set1->capacity * sizeof(uint64_t));
  for (size_t i = 0; i < set2->capacity; i++) {
    result->value[i] |= set2->value[i];
  }
}

void BitSetUnionInPlace(BitSet* dest, BitSet* src) {
  size_t max = dest->capacity;
  if (max < src->capacity) {
    max = src->capacity;
  }
  MakeRoom(dest, max);
  for (size_t i = 0; i < src->capacity; i++) {
    dest->value[i] |= src->value[i];
  }
}

void BitSetCopy(BitSet* to, BitSet* from) {
  MakeRoom(to, from->capacity);
  memcpy(to->value, from->value, from->capacity * sizeof(uint64_t));
}

bool BitSetEqual(BitSet* set1, BitSet* set2) {
  BitSet* larger = set2;
  size_t min = set1->capacity;
  if (min > set2->capacity) {
    min = set2->capacity;
    larger = set1;
  }

  // First compare area where the capacities match.  One may have a larger
  // capacity but all the extra bits might be zero.
  int v = memcmp(set1->value, set2->value, min * sizeof(uint64_t));
  if (v != 0) {
    return false;
  }
  // Now we look at the extra bits in the larger set to see if they
  // are all zero.
  for (size_t i = min; i < larger->capacity; i++) {
    if (larger->value[i] != 0) {
      return false;
    }
  }
  return true;
}

void BitSetExpand(BitSet* set, Vector* vec) {
  size_t index = 0;
  for (size_t word = 0; word < set->capacity; word++) {
    for (size_t bit = 0; bit < 64; bit++) {
      if ((set->value[word] & (1LL << bit)) != 0) {
        VectorAppend(vec, (void*)index);
      }
      index++;
    }
  }
}

size_t BitSetCount(BitSet* set) {
  size_t count = 0;
  for (size_t word = 0; word < set->capacity; word++) {
    count += (size_t)__builtin_popcountll(set->value[word]);
  }
  return count;
}

void BitSetPrint(BitSet* set, FILE* fp) {
  fprintf(fp, "{");
  const char* sep = "";
  size_t index = 0;
  for (size_t word = 0; word < set->capacity; word++) {
    for (size_t bit = 0; bit < 64; bit++) {
      if ((set->value[word] & (1LL << bit)) != 0) {
        fprintf(fp, "%s%zd", sep, index);
        sep = ", ";
      }
      index++;
    }
  }
  fprintf(fp, "}");
}

void BitSetIteratorStart(BitSetIterator* it, BitSet* set) {
  it->set = set;
  it->word_offset = 0;
  it->bit_offset = 0;
  while (it->word_offset < set->capacity &&
         set->value[it->word_offset] == 0) {
    it->word_offset++;
  }
  if (it->word_offset < set->capacity) {
    it->bit_offset =
        (size_t)__builtin_ctzll(set->value[it->word_offset]);
  }
}

bool BitSetIteratorDone(BitSetIterator* it);

void BitSetIteratorNext(BitSetIterator* it) {
  if (BitSetIteratorDone(it)) {
    return;
  }
  uint64_t remaining = 0;
  if (it->bit_offset < 63) {
    remaining =
        it->set->value[it->word_offset] &
        (UINT64_MAX << (it->bit_offset + 1));
  }
  if (remaining != 0) {
    it->bit_offset = (size_t)__builtin_ctzll(remaining);
    return;
  }
  do {
    it->word_offset++;
    if (it->word_offset < it->set->capacity &&
        it->set->value[it->word_offset] != 0) {
      it->bit_offset =
          (size_t)__builtin_ctzll(it->set->value[it->word_offset]);
      return;
    }
  } while (it->word_offset < it->set->capacity);
  it->bit_offset = 0;
}

size_t BitSetIteratorValue(BitSetIterator* it);
