//
//  hashset.c
//  c_compiler
//

#include "hashset.h"

#include <stdlib.h>

void HashSetInit(HashSet* set, size_t capacity) {
  size_t size = 16;
  while (size < capacity * 2) {
    size *= 2;
  }
  set->keys = calloc(size, sizeof(uint64_t));
  set->mask = size - 1;
  set->length = 0;
  set->has_zero_key = false;
}

void HashSetDestruct(HashSet* set) {
  free(set->keys);
  set->keys = NULL;
  set->mask = 0;
  set->length = 0;
  set->has_zero_key = false;
}

static void Grow(HashSet* set) {
  HashSet old = *set;
  HashSetInit(set, old.mask + 1);
  set->length = old.length;
  set->has_zero_key = old.has_zero_key;
  for (size_t i = 0; i <= old.mask; i++) {
    if (old.keys[i] != 0) {
      set->keys[HashSetFindSlot(set, old.keys[i])] = old.keys[i];
    }
  }
  HashSetDestruct(&old);
}

bool HashSetInsert(HashSet* set, uint64_t key) {
  if (key == 0) {
    bool is_new = !set->has_zero_key;
    set->has_zero_key = true;
    set->length += is_new;
    return is_new;
  }
  if (HashSetIsFull(set)) {
    Grow(set);
  }
  size_t slot = HashSetFindSlot(set, key);
  if (set->keys[slot] != 0) {
    return false;
  }
  set->keys[slot] = key;
  set->length++;
  return true;
}
