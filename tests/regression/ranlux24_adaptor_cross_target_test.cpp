// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>

int main() {
  std::ranlux24 engine24;
  unsigned value24 = 0;
  for (int i = 0; i < 24; ++i) value24 = engine24();
  if (value24 != 15059233U) return 1;
  std::ranlux48 engine48;
  unsigned long long value48 = 0;
  for (int i = 0; i < 12; ++i) value48 = engine48();
  if (value48 != 269312768919532ULL) return 2;
  return 0;
}
