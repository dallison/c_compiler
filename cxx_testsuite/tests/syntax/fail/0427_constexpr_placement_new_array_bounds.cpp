// RUN: -std=c++26
// EXPECT: placement new exceeds the bounds of its target storage

#include <new>

constexpr int oversized_array_placement() {
  int storage[2] = {};
  int* values = new (static_cast<void*>(&storage[0])) int[3];
  return values[0];
}

static_assert(oversized_array_placement() == 0);
