// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>
#include <sstream>

int main() {
  std::piecewise_constant_distribution<> distribution;
  if (distribution.intervals().size() != 2) return 1;
  if (distribution.densities().size() != 1) return 2;
  std::stringstream stream;
  stream << distribution;
  std::piecewise_constant_distribution<> restored;
  stream >> restored;
  if (!stream) return 3;
  return distribution == restored ? 0 : 4;
}
