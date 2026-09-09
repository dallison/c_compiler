// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <type_traits>

constexpr int detect() {
  if (std::is_constant_evaluated()) {
    return 1;
  }
  return 2;
}

static_assert(detect() == 1);

int main() {
  if (detect() != 2) {
    return 1;
  }
  int runtime = detect();
  if (runtime != 2) {
    return 2;
  }
  return 0;
}
