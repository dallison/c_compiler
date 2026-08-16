// RUN: -std=c++26

#include <utility>

using seq = std::integer_sequence<int, 1, 2, 3>;
static_assert(seq::size() == 3);
static_assert(std::tuple_size<seq>::value == 3);

constexpr int element = std::get<1>(seq{});
static_assert(element == 2);

int main() {
  int sum = 0;
  template for (constexpr int value : seq{}) {
    sum += value;
  }
  return sum == 6 ? 0 : 1;
}
