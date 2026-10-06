// RUN: -std=c++20 -fconstexpr-eval=pcode
// EXPECT: call to non-constexpr function 'memcpy'

// memcpy is not a constexpr function, even between objects of one type.
#include <cstring>

constexpr int copy() {
  int from = 7;
  int to = 0;
  std::memcpy(&to, &from, sizeof to);
  return to;
}

static_assert(copy() == 7);
