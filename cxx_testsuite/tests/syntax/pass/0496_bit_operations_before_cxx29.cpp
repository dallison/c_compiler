// RUN: -std=c++26

#include <bit>

#if __cpp_lib_bitops != 201907L
#error "__cpp_lib_bitops must retain its C++20 value before C++29"
#endif

static_assert(std::rotl(1u, 1) == 2u);

#include <version>

#if __cpp_lib_bitops != 201907L
#error "<version> must retain the C++20 bitops value before C++29"
#endif
