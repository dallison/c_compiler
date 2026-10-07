// RUN: -std=c++20 -fconstexpr-eval=pcode -fPIC

// Position-independent code generation (as for a Mach-O or shared object
// target) does not change constant evaluation: calls that pass the address of
// a local, and reads of namespace-scope constants, still fold.

struct P {
  int a;
  constexpr P(int x) : a(x) {}
};

constexpr int kBase = 40;
constexpr int kTable[] = {1, 2, 3};

constexpr int Deref(const int* p) { return *p; }
constexpr int& Forward(int& x) { return x; }
constexpr int Member(const P& p) { return p.a; }

constexpr int PointerArgument() {
  int y = 7;
  return Deref(&y);
}

constexpr int ReferenceResult() {
  int y = 2;
  Forward(y) = 9;
  return y;
}

constexpr int ClassArgument() {
  P p(6);
  return Member(p);
}

constexpr int Globals() { return kBase + kTable[2]; }

static_assert(PointerArgument() == 7);
static_assert(ReferenceResult() == 9);
static_assert(ClassArgument() == 6);
static_assert(Globals() == 43);
