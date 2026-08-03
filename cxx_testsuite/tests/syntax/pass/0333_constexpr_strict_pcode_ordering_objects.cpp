// RUN: -std=c++20 -fconstexpr-eval=pcode

#include <compare>

static_assert((1 <=> 2) == std::strong_ordering::less);
static_assert((2 <=> 1) == std::strong_ordering::greater);
static_assert((1 <=> 1) == std::strong_ordering::equal);
