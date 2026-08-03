// RUN: -std=c++20
// EXPECT: static_assert expression is not an integer constant expression

constexpr int labeled() {
label:
  return 7;
}

static_assert(labeled() == 7);
