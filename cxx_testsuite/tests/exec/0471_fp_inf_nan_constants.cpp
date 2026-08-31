// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <limits>

int main() {
  double inf = std::numeric_limits<double>::infinity();
  if (!(inf > 1e300)) return 1;
  if (inf != inf) return 2;

  float finf = std::numeric_limits<float>::infinity();
  if (!(finf > 1e30f)) return 3;

  double nan = std::numeric_limits<double>::quiet_NaN();
  if (nan == nan) return 4;

  return 0;
}
