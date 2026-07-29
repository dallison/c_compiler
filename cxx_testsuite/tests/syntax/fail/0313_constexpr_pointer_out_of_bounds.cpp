// RUN: -std=c++20
// EXPECT: static_assert expression is not an integer constant expression

constexpr bool invalid_pointer_arithmetic() {
  int values[2] = {1, 2};
  return &values[0] + 3 > values;
}

static_assert(invalid_pointer_arithmetic());
