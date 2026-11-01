// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cstdio>
#include <limits>

int main() {
  float narrow = 1.5f;
  if (std::sscanf("1e40", "%f", &narrow) != 1) return 1;
  if (narrow != std::numeric_limits<float>::infinity()) return 2;

  float normal = 0;
  if (std::sscanf("2.5", "%f", &normal) != 1 || normal != 2.5f) return 3;

  double huge = 1;
  if (std::sscanf("1e309", "%lf", &huge) != 1) return 4;
  if (huge != std::numeric_limits<double>::infinity()) return 5;

  double zero = 1;
  if (std::sscanf("0", "%lf", &zero) != 1 || zero != 0.0) return 6;

  double tiny = 1;
  if (std::sscanf("1e-1000", "%lf", &tiny) != 1 || tiny != 0.0) return 7;

  long double wide = 0;
  if (std::sscanf("9.5", "%Lf", &wide) != 1 || wide != 9.5L) return 8;

  double finite = 0;
  if (std::sscanf("1e20", "%lf", &finite) != 1 || finite < 9e19 ||
      finite > 1.1e20) {
    return 9;
  }
  return 0;
}
