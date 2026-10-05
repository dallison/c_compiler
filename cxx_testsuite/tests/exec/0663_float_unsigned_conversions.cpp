// RUN: -std=c++20
// EXPECT_EXIT: 0

// Conversions between floating point and unsigned integers produce integer
// results that must be usable as integers, including values above INT_MAX.

volatile float f = 3000000000.0f;
volatile double d = 4000000000.0;
volatile unsigned u = 4000000000u;

int main() {
  unsigned a = (unsigned)f;
  unsigned b = (unsigned)d;
  float c = (float)u;
  double e = (double)u;
  int s = (int)(f / 1000.0f);
  if (a != 3000000000u) return 1;
  if (b != 4000000000u) return 2;
  if (c != 4000000000.0f) return 3;
  if (e != 4000000000.0) return 4;
  if (s != 3000000) return 5;
  if ((unsigned)(f / 3.0f) + 1 != 1000000001u) return 6;
  return 0;
}
