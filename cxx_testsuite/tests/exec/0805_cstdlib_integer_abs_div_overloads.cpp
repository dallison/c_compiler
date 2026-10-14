// RUN: -std=c++17
// EXPECT_EXIT: 0

// <cstdlib> overloads std::abs and std::div for long and long long, so
// std::abs(int64_t) is not ambiguous between int and the floating overloads.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

int main() {
  int64_t secs = -42;
  int32_t nanos = -7;
  static_assert(std::is_same_v<decltype(std::abs(secs)), int64_t>);
  static_assert(std::is_same_v<decltype(std::abs(1L)), long>);
  static_assert(std::is_same_v<decltype(std::abs(1LL)), long long>);
  static_assert(std::is_same_v<decltype(std::abs(1)), int>);
  static_assert(std::is_same_v<decltype(std::abs(1.0)), double>);
  if (std::abs(secs) != 42) return 1;
  if (std::abs(nanos) != 7) return 2;
  if (std::abs(-5000000000LL) != 5000000000LL) return 3;
  if (std::abs(-3L) != 3L) return 4;
  if (std::abs(-2.5) != 2.5) return 5;
  std::ldiv_t ld = std::div(17L, 5L);
  if (ld.quot != 3 || ld.rem != 2) return 6;
  std::lldiv_t lld = std::div(-17LL, 5LL);
  if (lld.quot != -3 || lld.rem != -2) return 7;
  std::div_t d = std::div(9, 4);
  if (d.quot != 2 || d.rem != 1) return 8;
  return 0;
}
