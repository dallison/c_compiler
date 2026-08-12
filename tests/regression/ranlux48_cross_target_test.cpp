// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>

int main() {
  std::ranlux48_base engine;
  unsigned long long first = engine();
  if ((first & 0xffffffffULL) != 0xfce57b2cULL) return 1;
  if ((first >> 32) != 0x1555ULL) return 2;
  if (engine() != 28639057539807ULL) return 3;
  if (engine() != 276846226770426ULL) return 4;
  if (engine() != 130971693943559ULL) return 5;
  return 0;
}
