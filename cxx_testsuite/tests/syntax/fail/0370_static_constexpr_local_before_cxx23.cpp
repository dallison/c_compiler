// RUN: -std=c++20
// EXPECT: static_assert expression is not an integer constant expression

constexpr char hex_digit(int value) {
  static constexpr char digits[] = "0123456789abcdef";
  return digits[value];
}

static_assert(hex_digit(10) == 'a');
