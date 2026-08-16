// RUN: -target 6502 -std=c++23

#include <limits>
#include <stdfloat>
#include <type_traits>
#include <version>

#ifndef __STDCPP_FLOAT32_T__
#error "6502 must provide the binary32 extended type"
#endif
#ifdef __STDCPP_FLOAT64_T__
#error "6502 must not advertise the binary64 extended type"
#endif
#if __cpp_lib_stdfloat != 202306L
#error "__cpp_lib_stdfloat has the wrong value"
#endif

static_assert(sizeof(std::float32_t) == 4);
static_assert(!std::is_same_v<std::float32_t, float>);
static_assert(std::is_floating_point_v<std::float32_t>);
static_assert(std::numeric_limits<std::float32_t>::digits == 24);
static_assert(1.5f32 + 0.5f32 == 2.0f32);
