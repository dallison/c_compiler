// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <stdfloat>
#include <type_traits>

int main() {
#ifndef __cpp_lib_stdfloat
  return 1;
#endif

#ifdef __STDCPP_FLOAT32_T__
  static_assert(std::is_same_v<std::float32_t, decltype(0.0f32)>);
  std::float32_t value32 = 1.25f32;
  if (value32 + 2.0f32 != 3.25f32) return 2;
#endif

#ifdef __STDCPP_FLOAT64_T__
  static_assert(std::is_same_v<std::float64_t, decltype(0.0f64)>);
  std::float64_t value64 = 2.5f64;
  if (value64 * 2.0f64 != 5.0f64) return 3;
#endif

  return 0;
}
