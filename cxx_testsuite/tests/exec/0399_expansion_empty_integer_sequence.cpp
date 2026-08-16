// RUN: -std=c++26
// EXPECT_EXIT: 7

#include <utility>

int main() {
  int sum = 7;
  template for (constexpr int value : std::integer_sequence<int>{}) {
    sum += value;
  }
  return sum;
}
