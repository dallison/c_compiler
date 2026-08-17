// RUN: -std=c++23

#include <new>
#include <version>

#ifdef __cpp_lib_constexpr_new
#error "__cpp_lib_constexpr_new must not be defined before C++26"
#endif

#if __cpp_constexpr != 202211L
#error "__cpp_constexpr must retain its C++23 value"
#endif
