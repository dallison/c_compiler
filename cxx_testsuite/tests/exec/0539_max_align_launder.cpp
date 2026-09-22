// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <cstddef>
#include <new>

int main() {
  if (alignof(std::max_align_t) < alignof(long long)) {
    return 1;
  }
  int value = 7;
  int* p = std::launder(&value);
  return *p == 7 ? 0 : 2;
}
