// RUN: -std=c++26
// EXPECT: static_assert expression is not an integer constant expression

constexpr int positive(const int value)
    pre (value > 0) {
  return value;
}

constexpr int middle_violation() {
  return positive(0);
}

constexpr int nested_violation() {
  return middle_violation();
}

static_assert(nested_violation() == 0);
