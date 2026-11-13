//
//  hashmap.c
//  c_compiler
//

#include "hashmap.h"

#include <stdlib.h>

void HashMapInit(HashMap* map, size_t capacity) {
  HashSetInit(&map->set, capacity);
  map->values = malloc((map->set.mask + 1) * sizeof(int64_t));
  map->zero_value = 0;
}

void HashMapDestruct(HashMap* map) {
  HashSetDestruct(&map->set);
  free(map->values);
  map->values = NULL;
}

static void Grow(HashMap* map) {
  HashMap old = *map;
  HashMapInit(map, old.set.mask + 1);
  map->set.length = old.set.length;
  map->set.has_zero_key = old.set.has_zero_key;
  map->zero_value = old.zero_value;
  for (size_t i = 0; i <= old.set.mask; i++) {
    uint64_t key = old.set.keys[i];
    if (key != 0) {
      size_t slot = HashSetFindSlot(&map->set, key);
      map->set.keys[slot] = key;
      map->values[slot] = old.values[i];
    }
  }
  HashMapDestruct(&old);
}

bool HashMapInsert(HashMap* map, uint64_t key, int64_t value) {
  HashSet* set = &map->set;
  if (key == 0) {
    bool is_new = !set->has_zero_key;
    set->has_zero_key = true;
    set->length += is_new;
    map->zero_value = value;
    return is_new;
  }
  if (HashSetIsFull(set)) {
    Grow(map);
  }
  size_t slot = HashSetFindSlot(set, key);
  bool is_new = set->keys[slot] == 0;
  set->keys[slot] = key;
  map->values[slot] = value;
  set->length += is_new;
  return is_new;
}
