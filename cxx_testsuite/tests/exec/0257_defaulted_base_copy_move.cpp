// Implicitly-defaulted copy/move constructors and assignment operators of a
// derived class must copy/move the base subobject from the source, and
// copy-initialization (`T x = expr;`) of a base-having class must initialize
// the whole object at offset 0 rather than at its first declared member's
// offset.  Regression for base subobjects being lost / mis-offset during
// defaulted special-member synthesis and whole-object copy-initialization.

// EXPECT_EXIT: 42

struct Base {
  int b;
  explicit Base(int x) : b(x) {}
  Base(const Base&) = default;
  Base(Base&&) = default;
  Base& operator=(const Base&) = default;
  Base& operator=(Base&&) = default;
};

struct Der : Base {
  int d;
  Der(int x, int y) : Base(x), d(y) {}
  // All copy/move/assign are implicitly defaulted.
};

static Der make() { return Der(10, 5); }

static bool ok(const Der& v) { return v.b == 10 && v.d == 5; }

int main() {
  Der a(10, 5);
  Der ci = a;                               // copy-initialization
  Der pv = make();                          // prvalue copy-initialization
  Der di(a);                                // direct-init copy ctor
  Der mv(static_cast<Der&&>(Der(10, 5)));   // direct-init move ctor
  Der ca(0, 0);
  ca = a;                                   // copy assignment
  Der ma(0, 0);
  ma = static_cast<Der&&>(Der(10, 5));      // move assignment

  bool all = ok(ci) && ok(pv) && ok(di) && ok(mv) && ok(ca) && ok(ma);
  return all ? 42 : 1;
}
