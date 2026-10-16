// `sizeof(` followed by a type is a type-id only when the type-id ends at the
// `)`.  Explicit type conversions (`W<C>{c}`, `T(x)`) and member accesses on
// them are expressions.
struct P {
  int x;
  double y;
};
template <class F> struct W {
  F f;
};

template <class C> int measure(C c) { return (int)sizeof(W<C>{c}); }

int main() {
  int a[4];
  int n = 3;
  if (sizeof(W<double>{1.0}) != sizeof(double)) return 1;
  if (sizeof(P{1, 2}.x) != sizeof(int)) return 2;
  if (sizeof(P{1, 2}.y) != sizeof(double)) return 3;
  if (sizeof(int(n)) != sizeof(int)) return 4;
  if (sizeof(long(n)) != sizeof(long)) return 5;
  if (sizeof(P) != sizeof(P{})) return 6;
  if (sizeof(a) != 4 * sizeof(int) || sizeof (a)[0] != sizeof(int)) return 7;
  if (measure('c') != 1 || measure(2.0) != (int)sizeof(double)) return 8;
  return 0;
}
