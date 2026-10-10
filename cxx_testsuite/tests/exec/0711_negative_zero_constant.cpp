// RUN: -std=c++20
// EXPECT_EXIT: 0

// A function using both 0.0 and -0.0 must keep them as distinct constants:
// they compare equal, but the sign of zero is observable.
#include <cmath>
__attribute__((noinline)) double Pick(int which) {
  double positive = 0.0;
  double negative = -0.0;
  return which ? negative : positive;
}

__attribute__((noinline)) bool SignBit(double value) {
  return std::signbit(value);
}

int main() {
  if (SignBit(Pick(0))) return 1;
  if (!SignBit(Pick(1))) return 2;
  float fp = 0.0f;
  float fn = -0.0f;
  if (std::signbit(fp) || !std::signbit(fn)) return 3;
  if (1.0 / Pick(1) > 0.0) return 4;
  return 0;
}
