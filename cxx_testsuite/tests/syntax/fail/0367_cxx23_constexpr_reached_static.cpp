// RUN: -std=c++23
// EXPECT: static_assert expression is not an integer constant expression

constexpr int reached_static() {
  static int value = 4;
  return value;
}

static_assert(reached_static() == 4);
