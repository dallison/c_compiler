// RUN: -std=c++20
// EXPECT: bit builtin requires an unsigned integer operand

int invalid_bit_intrinsic(int value) {
  return __davecc_popcount(value, sizeof(value) * 8);
}
