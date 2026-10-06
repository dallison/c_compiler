// RUN: -std=c++20
// EXPECT: __builtin_bit_cast requires types of the same size

unsigned long long widen(unsigned value) {
  return __builtin_bit_cast(unsigned long long, value);
}
