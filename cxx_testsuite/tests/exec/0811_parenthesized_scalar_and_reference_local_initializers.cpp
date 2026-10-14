// RUN: -std=c++17
// EXPECT_EXIT: 0

// Direct-initialization with parentheses must store into scalar and reference
// locals exactly like copy-initialization with `=`.

#include <stdint.h>

enum E { kA = 4 };

struct S {
  int v;
};

int Four() { return 4; }

template <typename T>
T Sum(T v) {
  T x(v);
  T y(3);
  return x + y;
}

int main() {
  int x(5);
  bool b(true);
  char ch('a');
  double d(2.5);
  const int n(3);
  int* p(&x);
  auto a(9);
  E e(kA);
  long l(x + 1);
  static int s(11);
  uint32_t u(static_cast<uint32_t>(x));
  if (x != 5 || !b || ch != 'a' || d != 2.5 || n != 3) return 1;
  if (*p != 5 || a != 9 || e != kA || l != 6 || s != 11 || u != 5u) return 2;

  const int& cr(x);
  int& r(x);
  r = 8;
  if (cr != 8 || x != 8) return 3;
  const int& temp(Four());
  if (temp != 4) return 4;
  int&& rr(static_cast<int&&>(x));
  if (rr != 8) return 5;

  S obj{3};
  S& sr(obj);
  const S& csr(obj);
  sr.v = 6;
  if (csr.v != 6) return 6;

  if (Sum<int>(4) != 7 || Sum<long>(1) != 4) return 7;

  int loops = 0;
  for (bool go(true); go; go = false) loops++;
  if (loops != 1) return 8;
  return 0;
}
