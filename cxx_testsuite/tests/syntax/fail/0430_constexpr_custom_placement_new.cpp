// RUN: -std=c++26
// EXPECT: selected placement allocation function is not permitted in a constant expression

#include <new>

constexpr void* operator new(unsigned long, int, void* pointer) {
  return pointer;
}

constexpr int custom_placement() {
  int storage = 0;
  int* value = new (0, &storage) int(42);
  return *value;
}

static_assert(custom_placement() == 42);
