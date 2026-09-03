// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <complex>
#include <sstream>

static bool near(double left, double right) {
  double difference = left - right;
  return difference > -0.001 && difference < 0.001;
}

int main() {
  std::complex<double> a(3.0, 4.0);
  std::complex<double> b(1.0, -2.0);

  std::complex<double> sum = a + b;
  if (!near(sum.real(), 4.0) || !near(sum.imag(), 2.0)) {
    return 1;
  }

  std::complex<double> product = a * b;
  if (!near(product.real(), 11.0) || !near(product.imag(), -2.0)) {
    return 2;
  }

  std::complex<double> quotient = product / b;
  if (!near(quotient.real(), 3.0) || !near(quotient.imag(), 4.0)) {
    return 3;
  }
  if (!near(std::abs(a), 5.0) || !near(std::norm(a), 25.0)) {
    return 4;
  }

  std::complex<double> root = std::sqrt(std::complex<double>(-4.0, 0.0));
  if (!near(root.real(), 0.0) || !near(root.imag(), 2.0)) {
    return 5;
  }

  std::complex<double> unit = std::exp(std::complex<double>(0.0, 0.0));
  if (!near(unit.real(), 1.0) || !near(unit.imag(), 0.0)) {
    return 6;
  }
  std::complex<double> zero = std::log(unit);
  if (!near(zero.real(), 0.0) || !near(zero.imag(), 0.0)) {
    return 7;
  }

  std::stringstream stream;
  stream << a;
  std::complex<double> parsed;
  stream >> parsed;
  if (parsed != a) {
    return 8;
  }
  return 0;
}
