// RUN: -std=c++20
// EXPECT_EXIT: 0

// A temporary created in one arm of ?: or on the right of && / || is destroyed
// at the end of the full-expression only if it was constructed.  The
// temporaries of a return statement's operand are destroyed before the
// function returns, while the returned object itself survives.
int live = 0;
int constructed = 0;

struct T {
  int v;
  T(int x) : v(x) { ++live; ++constructed; }
  T(const T& o) : v(o.v) { ++live; ++constructed; }
  ~T() { --live; }
  bool ok() const { return v != 0; }
  int get() const { return v; }
};

int pick(bool c) { return c ? T(1).get() : T(2).get() + T(3).get(); }
bool both(bool a) { return a && T(1).ok(); }
bool either(bool a) { return a || T(0).ok(); }
T make(int v) { return T(T(v).get() + T(1).get()); }
int twice(const T& t) { return t.get() * 2; }
int nested(int v) { return twice(T(v)) + twice(T(v + 1)); }

int main() {
  if (pick(true) != 1 || live != 0 || constructed != 1) return 1;
  constructed = 0;
  if (pick(false) != 5 || live != 0 || constructed != 2) return 2;
  constructed = 0;
  if (both(false) || live != 0 || constructed != 0) return 3;
  if (!both(true) || live != 0 || constructed != 1) return 4;
  constructed = 0;
  if (!either(true) || live != 0 || constructed != 0) return 5;
  if (either(false) || live != 0 || constructed != 1) return 6;
  {
    T t = make(4);
    if (t.get() != 5 || live != 1) return 7;
  }
  if (live != 0) return 8;
  if (nested(3) != 14 || live != 0) return 9;
  return 0;
}
