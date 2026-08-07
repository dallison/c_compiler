// RUN: -std=c++20

#include <numbers>
#include <type_traits>

#if __cpp_lib_math_constants != 201907L
#error "__cpp_lib_math_constants has the wrong value"
#endif

static_assert(std::is_same_v<decltype(std::numbers::pi), const double>);
static_assert(std::numbers::pi > 3.14159 &&
              std::numbers::pi < 3.14160);
static_assert(std::numbers::e_v<float> > 2.7182f &&
              std::numbers::e_v<float> < 2.7183f);
static_assert(std::numbers::sqrt2_v<long double> > 1.4142L &&
              std::numbers::sqrt2_v<long double> < 1.4143L);
static_assert(std::numbers::phi > 1.6180 &&
              std::numbers::phi < 1.6181);

template <class T>
constexpr T circle(T radius) {
  return std::numbers::pi_v<T> * radius * radius;
}

static_assert(circle(2.0) > 12.5663 && circle(2.0) < 12.5664);

int main() {
  return 0;
}
