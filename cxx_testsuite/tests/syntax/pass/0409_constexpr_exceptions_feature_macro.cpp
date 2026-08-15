// RUN: -std=c++26

#include <exception>
#include <version>

#if __cpp_lib_constexpr_exceptions != 202502L
#error "__cpp_lib_constexpr_exceptions has the wrong C++26 value"
#endif
