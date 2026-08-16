// RUN: -std=c++23

#include <limits>
#include <stdfloat>
#include <type_traits>
#include <version>

#if __STDCPP_FLOAT32_T__ != 1
#error "__STDCPP_FLOAT32_T__ must advertise binary32 support"
#endif
#if __STDCPP_FLOAT64_T__ != 1
#error "__STDCPP_FLOAT64_T__ must advertise binary64 support"
#endif
#if __cpp_lib_stdfloat != 202306L
#error "__cpp_lib_stdfloat has the wrong value"
#endif

static_assert(sizeof(std::float32_t) == 4);
static_assert(sizeof(std::float64_t) == 8);
static_assert(!std::is_same_v<std::float32_t, float>);
static_assert(!std::is_same_v<std::float64_t, double>);
static_assert(std::is_floating_point_v<std::float32_t>);
static_assert(std::is_floating_point_v<const std::float64_t>);

static_assert(std::is_same_v<decltype(1.0f32), std::float32_t>);
static_assert(std::is_same_v<decltype(1.0F32), std::float32_t>);
static_assert(std::is_same_v<decltype(1.0f64), std::float64_t>);
static_assert(std::is_same_v<decltype(1.0F64), std::float64_t>);

static_assert(std::is_same_v<decltype(1.0f32 + 2.0f32), std::float32_t>);
static_assert(std::is_same_v<decltype(1.0f64 + 2.0f64), std::float64_t>);
static_assert(std::is_same_v<decltype(1.0f32 + 2.0f), float>);
static_assert(std::is_same_v<decltype(1.0f64 + 2.0), double>);
static_assert(std::is_same_v<decltype(1.0f32 + 2.0f64), std::float64_t>);

constexpr std::float64_t widen(std::float32_t value) { return value; }
constexpr std::float32_t narrow(std::float64_t value) {
  return static_cast<std::float32_t>(value);
}

static_assert(widen(1.25f32) == 1.25f64);
static_assert(narrow(2.5f64) == 2.5f32);
static_assert(std::numeric_limits<std::float32_t>::is_specialized);
static_assert(std::numeric_limits<std::float32_t>::digits == 24);
static_assert(std::numeric_limits<std::float64_t>::digits == 53);

constexpr int select(float) { return 1; }
constexpr int select(std::float32_t) { return 2; }
constexpr int select(double) { return 3; }
constexpr int select(std::float64_t) { return 4; }

static_assert(select(0.0f32) == 2);
static_assert(select(0.0f64) == 4);
