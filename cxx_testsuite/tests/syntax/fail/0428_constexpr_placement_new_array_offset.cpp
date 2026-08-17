// RUN: -std=c++26
// EXPECT: placement new exceeds the bounds of its target storage

#include <new>

constexpr int offset_array_placement() {
  int storage[3] = {};
  int* values = new (static_cast<void*>(&storage[1])) int[3];
  return values[0];
}

static_assert(offset_array_placement() == 0);
