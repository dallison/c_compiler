// RUN: -std=c++26

constexpr int divide(int numerator, int denominator) {
  if (denominator == 0) {
    throw 42;
  }
  return numerator / denominator;
}

constexpr int safe_divide(int numerator, int denominator) {
  try {
    return divide(numerator, denominator);
  } catch (int value) {
    return -value;
  }
}

constexpr int throw_from_expression(bool should_throw) {
  try {
    return should_throw ? throw 7 : 9;
  } catch (int value) {
    return value;
  }
}

static_assert(safe_divide(10, 2) == 5);
static_assert(safe_divide(10, 0) == -42);
static_assert(throw_from_expression(false) == 9);
static_assert(throw_from_expression(true) == 7);
