// RUN: -std=c++26
// EXPECT_EXIT: 6

#include <array>

int main() {
  constexpr std::array<int, 3> values{{1, 2, 3}};
  int sum = 0;
  template for (constexpr int value : values) {
    sum += value;
  }
  return sum;
}
