// RUN: -std=c++26
// EXPECT_EXIT: 3

#include <utility>

int main() {
  int sum = 0;
  template for (constexpr auto index : std::index_sequence<0, 1, 2>{}) {
    sum += (int)index;
  }
  return sum;
}
