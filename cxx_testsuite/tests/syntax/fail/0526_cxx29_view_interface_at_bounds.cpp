// RUN: -std=c++29 -fexceptions
// EXPECT: static_assert expression is not an integer constant expression

#include <ranges>

constexpr bool invalid_view_access() {
  int values[] = {1, 2};
  std::ranges::subrange view(values, values + 2);
  return view.at(-1) == 0;
}

static_assert(invalid_view_access());
