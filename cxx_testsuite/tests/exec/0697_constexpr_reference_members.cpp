// A reference member of a constexpr object designates the object it is bound
// to: writes through it reach that object, and constant evaluation reads a
// const referent's value.  Reference members also work inside constexpr
// functions, whether bound by a constructor or by aggregate initialization.
int g = 5;
const int kc = 6;
double gd = 1.5;

struct R {
  int d;
  int& r;
};
struct CR {
  int d;
  const int& r;
};
struct DR {
  double& r;
  int tail;
};
struct W {
  int& r;
  constexpr W(int& x) : r(x) {}
};

constexpr CR k1{1, kc};
static_assert(k1.d == 1 && k1.r == 6);
static_assert(&k1.r == &kc);
constexpr R k2{2, g};
static_assert(&k2.r == &g);
constexpr DR k3{gd, 3};
static_assert(&k3.r == &gd && k3.tail == 3);

constexpr int ByConstructor() {
  int x = 1;
  W w(x);
  w.r = 7;
  return x;
}
static_assert(ByConstructor() == 7);

constexpr int ReadAggregate() {
  int x = 1;
  R l{2, x};
  return l.r;
}
static_assert(ReadAggregate() == 1);

constexpr int WriteAggregate() {
  int x = 1;
  R l{2, x};
  l.r = 7;
  return x;
}
static_assert(WriteAggregate() == 7);

constexpr int RebindLocal() {
  int x = 1;
  R l = {2, x};
  int& q = l.r;
  q = 5;
  return x;
}
static_assert(RebindLocal() == 5);

int main() {
  k2.r = 9;
  if (g != 9) return 1;
  if (k1.r != 6) return 2;
  k3.r = 4.25;
  if (gd != 4.25) return 3;
  W w(g);
  w.r += 1;
  if (g != 10) return 4;
  return 0;
}
