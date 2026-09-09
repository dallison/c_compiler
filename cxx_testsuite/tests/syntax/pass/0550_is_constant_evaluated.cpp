// RUN: -std=c++20

#include <type_traits>
#include <version>

#if __cpp_lib_is_constant_evaluated != 201811L
#error "__cpp_lib_is_constant_evaluated must be 201811L in C++20"
#endif

constexpr bool from_builtin() {
  return __builtin_is_constant_evaluated();
}

constexpr bool from_library() {
  return std::is_constant_evaluated();
}

static_assert(from_builtin());
static_assert(from_library());
static_assert(std::is_constant_evaluated());
