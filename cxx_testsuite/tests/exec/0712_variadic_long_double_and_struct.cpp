// RUN: -std=c++20
// EXPECT_EXIT: 0

// Variadic struct and long double arguments are passed by value, whether the
// value is a local or a parameter, and va_arg reads them back in place.
#include <cstdarg>
#include <cstdio>

struct P {
  long a;
  int b;
};

static long Sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  long s = 0;
  for (int i = 0; i < n; i++) {
    P p = va_arg(ap, P);
    s += p.a * 10 + p.b;
    long double d = va_arg(ap, long double);
    s += (long)d;
  }
  va_end(ap);
  return s;
}

__attribute__((noinline)) long Forward(long double value) {
  P p = {5, 6};
  return Sum(1, p, value);
}

__attribute__((noinline)) int Format(long double value, char* out, int size) {
  return std::snprintf(out, size, "%.2Lf", value);
}

int main() {
  P p1 = {1, 2};
  P p2 = {3, 4};
  long double a = 100.0L;
  long double b = 2000.0L;
  if (Sum(2, p1, a, p2, b) != 12 + 100 + 34 + 2000) return 1;
  if (Forward(300.0L) != 56 + 300) return 2;
  char buffer[32];
  if (Format(1.25L, buffer, sizeof(buffer)) != 4) return 3;
  if (buffer[0] != '1' || buffer[2] != '2' || buffer[3] != '5') return 4;
  return 0;
}
