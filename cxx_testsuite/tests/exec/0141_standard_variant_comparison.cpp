// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

int main(void) {
  std::variant<int, long> one(1);
  std::variant<int, long> another_one(1);
  std::variant<int, long> two(2);
  std::variant<int, long> long_zero(std::in_place_index<1>, 0L);

  if (!(one == another_one) || one != another_one) {
    return 1;
  }
  if (!(one < two) || !(two > one) || !(one <= another_one) ||
      !(one >= another_one)) {
    return 2;
  }
  if (!(two < long_zero) || !(long_zero > two)) {
    return 3;
  }

  std::strong_ordering same = one <=> another_one;
  std::strong_ordering less_value = one <=> two;
  std::strong_ordering less_index = two <=> long_zero;
  if (same != std::strong_ordering::equal ||
      less_value != std::strong_ordering::less ||
      less_index != std::strong_ordering::less) {
    return 4;
  }

  return 0;
}
