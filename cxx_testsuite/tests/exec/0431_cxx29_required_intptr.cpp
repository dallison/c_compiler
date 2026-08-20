// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <atomic>
#include <cstdint>

int main() {
  int object = 42;

  std::uintptr_t unsigned_value =
      reinterpret_cast<std::uintptr_t>(&object);
  int* unsigned_round_trip =
      reinterpret_cast<int*>(unsigned_value);
  if (unsigned_round_trip != &object || *unsigned_round_trip != 42) {
    return 1;
  }

  std::intptr_t signed_value =
      reinterpret_cast<std::intptr_t>(&object);
  int* signed_round_trip = reinterpret_cast<int*>(signed_value);
  if (signed_round_trip != &object || *signed_round_trip != 42) {
    return 2;
  }

  std::atomic_uintptr_t atomic_value(unsigned_value);
  if (atomic_value.load() != unsigned_value) {
    return 3;
  }
  return 0;
}
