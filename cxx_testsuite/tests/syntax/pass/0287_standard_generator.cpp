// RUN: -std=c++23

#include <generator>
#include <ranges>
#include <type_traits>

#if __cpp_lib_generator != 202207L
#error "__cpp_lib_generator has the wrong value"
#endif

std::generator<int> values() {
  co_yield 1;
}

static_assert(std::ranges::range<std::generator<int>>);
static_assert(std::ranges::input_range<std::generator<int>>);
static_assert(std::is_nothrow_destructible_v<std::generator<int>>);
static_assert(std::movable<std::generator<int>>);
static_assert(std::ranges::view<std::generator<int>>);
