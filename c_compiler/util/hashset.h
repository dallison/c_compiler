//
//  hashset.h
//  c_compiler
//

#ifndef hashset_h
#define hashset_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A hash set of 64-bit keys.  Pointers can be used as keys by casting them
// through uintptr_t.  Insertion and lookup take constant time on average, so
// prefer this to Set, whose insertion moves every later element, when there
// may be many keys.  Keys cannot be removed.  HashMap adds a value to each key.
typedef struct {
  uint64_t* keys;     // Open addressing; 0 marks an empty slot.
  size_t mask;        // Number of slots minus one.
  size_t length;      // Number of keys, including key 0.
  bool has_zero_key;  // Key 0 is held here rather than in a slot.
} HashSet;

// |capacity| is the number of keys expected; the set grows past it as needed.
void HashSetInit(HashSet* set, size_t capacity);
void HashSetDestruct(HashSet* set);

// Returns true if the key is new.
bool HashSetInsert(HashSet* set, uint64_t key);

static inline size_t HashSetHash(uint64_t key) {
  key ^= key >> 33;
  key *= UINT64_C(0xff51afd7ed558ccd);
  key ^= key >> 33;
  return (size_t)key;
}

// The slot holding the nonzero |key|, or the empty slot where it would go.
static inline size_t HashSetFindSlot(const HashSet* set, uint64_t key) {
  size_t i = HashSetHash(key) & set->mask;
  while (set->keys[i] != key && set->keys[i] != 0) {
    i = (i + 1) & set->mask;
  }
  return i;
}

static inline bool HashSetContains(const HashSet* set, uint64_t key) {
  return key == 0 ? set->has_zero_key
                  : set->keys[HashSetFindSlot(set, key)] != 0;
}

// Whether inserting one more key should first double the slots, keeping at
// least half of them empty.
static inline bool HashSetIsFull(const HashSet* set) {
  return (set->length + 1) * 2 > set->mask + 1;
}

#endif /* hashset_h */
