// RUN: -std=c++26
// EXPECT_EXIT: 12

#include <utility>

template <int... Values>
constexpr int sum_sequence() {
  int sum = 0;
  template for (constexpr int value : std::integer_sequence<int, Values...>{}) {
    sum += value;
  }
  return sum;
}

int main() {
  return sum_sequence<3, 4, 5>();
}
