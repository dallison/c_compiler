// RUN: -std=c++20
// EXPECT: static_assert expression is not an integer constant expression

constexpr int checked_value(int value) {
  try {
    if (value < 0) {
      throw value;
    }
    return value;
  } catch (...) {
    return 0;
  }
}

static_assert(checked_value(-1) == 0);
