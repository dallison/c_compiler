// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cmath>

static bool close(double left, double right, double tolerance) {
  return std::fabs(left - right) <= tolerance;
}

int main() {
  if (!close(std::sinh(1.0), 1.1752011936, 1e-5)) return 1;
  if (!close(std::cosh(1.0), 1.5430806348, 1e-5)) return 12;
  if (!close(std::tanh(1.0), 0.7615941559, 1e-5)) return 13;
  if (!close(std::log2(8.0), 3.0, 1e-5) ||
      !close(std::log10(1000.0), 3.0, 1e-5) ||
      !close(std::cbrt(27.0), 3.0, 1e-5)) return 2;
  if (!close(std::hypot(3.0, 4.0), 5.0, 1e-5) ||
      !close(std::erf(1.0), 0.84270079, 2e-6)) return 3;
  if (std::trunc(-2.9) != -2.0 || std::round(-2.5) != -3.0 ||
      std::rint(2.5) != 2.0 || std::rint(3.5) != 4.0) return 4;
  if (std::fmod(7.0, 4.0) != 3.0 ||
      std::remainder(7.0, 4.0) != -1.0) return 5;
  if (std::fmax(NAN, 3.0) != 3.0 || std::fmin(2.0, NAN) != 2.0 ||
      std::fdim(5.0, 3.0) != 2.0) return 6;
  if (!isnan(NAN) || !isinf(INFINITY) || !isfinite(1.0) ||
      !signbit(-0.0) || fpclassify(0.0) != FP_ZERO) return 7;
  if (std::nextafter(1.0, 2.0) <= 1.0 ||
      std::copysign(3.0, -1.0) != -3.0) return 8;
  if (!close(std::tgamma(5.0), 24.0, 1e-5) ||
      !close(std::lgamma(5.0), std::log(24.0), 1e-5)) return 9;
  return 0;
}
