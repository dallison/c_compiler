// RUN: -std=c++29
// EXPECT: static_assert expression is not an integer constant expression

#include <bit>

static_assert(std::bit_repeat(1u, 0) == 0);
