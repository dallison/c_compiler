// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <cmath>

int main() {
  long double value = 0.5L;
  int category = std::fpclassify(value);
  bool negative = std::signbit(-1.0L);
  int exp = 0;
  double mantissa = static_cast<double>(std::frexp(value, &exp));
  if (category != FP_NORMAL) {
    return 1;
  }
  if (!negative) {
    return 2;
  }
  if (mantissa == 0.0) {
    return 3;
  }
  if (std::fpclassify(0) != FP_ZERO) {
    return 4;
  }
  return 0;
}
