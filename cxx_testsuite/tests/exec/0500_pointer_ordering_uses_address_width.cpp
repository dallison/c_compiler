// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cstdint>

int main() {
  const std::uintptr_t sign_bit =
      std::uintptr_t(1) << (sizeof(std::uintptr_t) * 8 - 1);
  auto* lower = reinterpret_cast<unsigned char*>(sign_bit - 1);
  auto* upper = reinterpret_cast<unsigned char*>(sign_bit);

  bool less = lower < upper;
  bool greater = upper > lower;
  if (!less || !greater) {
    return 1;
  }
  if (upper <= lower || lower >= upper) {
    return 2;
  }
  return 0;
}
