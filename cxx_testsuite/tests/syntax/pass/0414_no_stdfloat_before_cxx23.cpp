// RUN: -std=c++20

#include <stdfloat>
#include <version>

#ifdef __STDCPP_FLOAT32_T__
#error "__STDCPP_FLOAT32_T__ must not be defined before C++23"
#endif
#ifdef __STDCPP_FLOAT64_T__
#error "__STDCPP_FLOAT64_T__ must not be defined before C++23"
#endif
#ifdef __cpp_lib_stdfloat
#error "__cpp_lib_stdfloat must not be defined before C++23"
#endif
