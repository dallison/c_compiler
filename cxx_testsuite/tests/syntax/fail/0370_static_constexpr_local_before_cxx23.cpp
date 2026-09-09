// RUN: -std=c++20
// EXPECT: static or thread_local variable in a constexpr function requires C++23

constexpr char hex_digit(int value) {
  static constexpr char digits[] = "0123456789abcdef";
  return digits[value];
}

static_assert(hex_digit(10) == 'a');
