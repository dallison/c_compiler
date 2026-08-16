// RUN: -std=c++26
// EXPECT_EXIT: 6

#include <utility>

int main() {
  using seq = std::integer_sequence<int, 1, 2, 3>;
  int sum = 0;
  template for (constexpr int value : seq{}) {
    sum += value;
  }
  return sum;
}
