// RUN: -std=c++20 -fconstexpr-eval=ast
// EXPECT: static_assert expression is not an integer constant expression

// memcmp is not a constexpr function.
#include <cstring>

constexpr int order() {
  char a[2] = {1, 2};
  char b[2] = {1, 3};
  return std::memcmp(a, b, 2);
}

static_assert(order() < 0);
