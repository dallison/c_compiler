// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <complex>
#include <type_traits>
#include <version>

#if __cpp_lib_constexpr_complex < 201711L
#error constexpr complex support is not advertised
#endif

#if __cpp_lib_complex_udls < 201309L
#error complex literals are not advertised
#endif

#if __cpp_lib_tuple_like < 202311L
#error C++26 complex tuple protocol is not advertised
#endif

constexpr std::complex<double> base(3.0, 4.0);
constexpr std::complex<double> sum = base + std::complex<double>(1.0, -2.0);
constexpr std::complex<double> product =
    base * std::complex<double>(1.0, -2.0);

static_assert(base.real() == 3.0 && base.imag() == 4.0);
static_assert(sum.real() == 4.0 && sum.imag() == 2.0);
static_assert(product.real() == 11.0 && product.imag() == -2.0);
static_assert(std::norm(base) == 25.0);
static_assert(std::conj(base).imag() == -4.0);
static_assert(std::is_same<decltype(std::real(1)), double>::value);
static_assert(std::is_same<decltype(std::imag(1.0f)), float>::value);
static_assert(
    std::is_same<decltype(std::conj(1)), std::complex<double> >::value);
static_assert(
    std::is_same<decltype(std::proj(1.0f)), std::complex<float> >::value);
static_assert(std::tuple_size<std::complex<double> >::value == 2);
static_assert(
    std::is_same<std::tuple_element<0, std::complex<double> >::type,
                 double>::value);
static_assert(
    std::is_same<std::tuple_element<1, std::complex<double> >::type,
                 double>::value);
static_assert(std::tuple_size<std::complex<int> >::value == 2);
static_assert(
    std::is_same<
        decltype(std::pow(std::complex<float>(), std::complex<double>())),
        std::complex<double> >::value);
static_assert(
    std::is_same<decltype(std::pow(std::complex<float>(), 2.0)),
                 std::complex<double> >::value);
static_assert(
    std::is_same<decltype(std::pow(std::complex<float>(), 2)),
                 std::complex<float> >::value);
static_assert(
    std::is_same<decltype(std::pow(2.0f, std::complex<double>())),
                 std::complex<double> >::value);
static bool near(double left, double right) {
  double difference = left - right;
  return difference > -0.0001 && difference < 0.0001;
}

int main() {
  std::complex<double> tuple_value(2.0, 5.0);
  std::get<0>(tuple_value) = 7.0;
  std::get<1>(tuple_value) = -3.0;
  if (tuple_value != std::complex<double>(7.0, -3.0)) return 1;

  std::complex<double> inverse =
      std::asinh(std::complex<double>(0.0, 0.0));
  if (!near(inverse.real(), 0.0) || !near(inverse.imag(), 0.0)) return 2;

  inverse = std::acosh(std::complex<double>(1.0, 0.0));
  if (!near(inverse.real(), 0.0) || !near(inverse.imag(), 0.0)) return 3;

  inverse = std::atanh(std::complex<double>(0.0, 0.0));
  if (!near(inverse.real(), 0.0) || !near(inverse.imag(), 0.0)) return 4;
  const double large = 1.0e300;
  std::complex<double> quotient =
      std::complex<double>(large, large) /
      std::complex<double>(large, large);
  if (!near(quotient.real(), 1.0) || !near(quotient.imag(), 0.0)) return 5;

  std::complex<double> projected =
      std::proj(std::complex<double>(INFINITY, -2.0));
  if (!isinf(projected.real()) || projected.real() < 0.0) return 6;
  if (projected.imag() != 0.0 || !signbit(projected.imag())) return 7;

  std::complex<double> root =
      std::sqrt(std::complex<double>(-4.0, -0.0));
  if (!near(root.real(), 0.0) || !near(root.imag(), -2.0)) return 8;

  std::complex<float> float_quotient =
      std::complex<float>(1.0e30f, 1.0e30f) /
      std::complex<float>(1.0e30f, 1.0e30f);
  if (!near(float_quotient.real(), 1.0) ||
      !near(float_quotient.imag(), 0.0)) return 9;

  using namespace std::complex_literals;
  constexpr std::complex<double> literal = 2.0 + 3.0i;
  static_assert(literal.real() == 2.0 && literal.imag() == 3.0);

  return std::tuple_size<std::complex<double> >::value == 2 ? 0 : 10;
}
