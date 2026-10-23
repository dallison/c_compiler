// RUN: -std=c++17
// EXPECT_EXIT: 0

int main() {
  int value = 1;
  if (__builtin_clz(value) != 31) return 1;
  if (__builtin_clz(value - 1 + 1) != 31) return 2;
  unsigned int wide = 0x80000000u;
  if (__builtin_clz(wide) != 0) return 3;
  return 0;
}
