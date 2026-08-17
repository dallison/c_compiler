// RUN: -std=c++26
// EXPECT: placement new target is not a constant address

#include <new>

int external_storage;

constexpr int replace_external_storage() {
  new (static_cast<void*>(&external_storage)) int(42);
  return external_storage;
}

static_assert(replace_external_storage() == 42);
