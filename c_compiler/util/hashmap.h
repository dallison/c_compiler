//
//  hashmap.h
//  c_compiler
//

#ifndef hashmap_h
#define hashmap_h

#include "hashset.h"

// A hash map from 64-bit keys to 64-bit values: a HashSet of the keys plus
// their values.  Prefer this to Map, whose insertion moves every later key,
// when there may be many keys.  Keys cannot be removed.
typedef struct {
  HashSet set;
  int64_t* values;  // Parallel to set.keys.
  int64_t zero_value;
} HashMap;

// |capacity| is the number of keys expected; the map grows past it as needed.
void HashMapInit(HashMap* map, size_t capacity);
void HashMapDestruct(HashMap* map);

// Inserts the key or replaces its value.  Returns true if the key is new.
bool HashMapInsert(HashMap* map, uint64_t key, int64_t value);

static inline bool HashMapContains(const HashMap* map, uint64_t key) {
  return HashSetContains(&map->set, key);
}

// Returns the key's value, or |absent| if the key is not in the map.
static inline int64_t HashMapFind(const HashMap* map, uint64_t key,
                                  int64_t absent) {
  if (key == 0) {
    return map->set.has_zero_key ? map->zero_value : absent;
  }
  size_t slot = HashSetFindSlot(&map->set, key);
  return map->set.keys[slot] != 0 ? map->values[slot] : absent;
}

#endif /* hashmap_h */
