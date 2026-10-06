// RUN: -std=c++20 -fconstexpr-eval=ast
// EXPECT: static_assert expression is not an integer constant expression

// A constant expression may not copy an int's bytes into a char array with
// memcpy.
#include <cstring>

constexpr int first_byte() {
  int value = 1;
  char bytes[sizeof value];
  std::memcpy(bytes, &value, sizeof value);
  return bytes[0];
}

static_assert(first_byte() == 1);
