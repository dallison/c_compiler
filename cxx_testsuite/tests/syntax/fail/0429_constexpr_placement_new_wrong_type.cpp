// RUN: -std=c++26
// EXPECT: placement new target does not point to an object of the allocated type

#include <new>

constexpr int wrong_placement_type() {
  long storage = 0;
  int* value = new (static_cast<void*>(&storage)) int(42);
  return *value;
}

static_assert(wrong_placement_type() == 42);
