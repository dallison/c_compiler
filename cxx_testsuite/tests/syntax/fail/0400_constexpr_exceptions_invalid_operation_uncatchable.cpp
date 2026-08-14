// RUN: -std=c++26
// EXPECT: static_assert expression is not an integer constant expression

constexpr int invalid_operations_are_not_exceptions() {
  try {
    int zero = 0;
    return 1 / zero;
  } catch (...) {
    return 0;
  }
}

static_assert(invalid_operations_are_not_exceptions() == 0);
