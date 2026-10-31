// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <ratio>

int main() {
  using large = std::ratio<4611686018427387904LL, 3>;
  using smaller = std::ratio<4611686018427387904LL, 5>;
  if (std::ratio_less<large, smaller>::value) return 1;
  if (!std::ratio_less<smaller, large>::value) return 2;

  using reduced = std::ratio<2, 4>;
  if (reduced::num != 1 || reduced::den != 2) return 3;
  using negative = std::ratio<1, -2>;
  if (negative::num != -1 || negative::den != 2) return 4;
  if (!std::ratio_less<negative, reduced>::value) return 5;
  if (!std::ratio_equal<std::ratio<1, 2>, reduced>::value) return 6;
  return 0;
}
