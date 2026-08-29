// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Returning a call whose result still has to be converted must not build that
// call's result in the caller's return slot.  What the caller gets back is the
// conversion's output, so the call has to leave its own result where the
// conversion can read it.  When optimizing, the call was marked for return
// value optimization anyway; it wrote over the return slot and the conversion
// then read an uninitialized temporary.  std::unordered_set::insert returns
// pair<iterator, bool> by converting the table's pair<mutable_iterator, bool>,
// which is where this showed up.

struct Inner {
  long a, b, c, d;
  Inner() : a(0), b(0), c(0), d(0) {}
  Inner(long a_, long b_, long c_, long d_) : a(a_), b(b_), c(c_), d(d_) {}
};

struct Outer {
  long a, b, c, d;
  bool flag;
  Outer() : a(0), b(0), c(0), d(0), flag(false) {}
  // The converting constructor the return has to go through.
  Outer(const Inner& other, bool f)
      : a(other.a), b(other.b), c(other.c), d(other.d), flag(f) {}
};

struct Pair {
  Inner first;
  bool second;
  Pair(const Inner& f, bool s) : first(f), second(s) {}
  operator Outer() const { return Outer(first, second); }
};

__attribute__((noinline)) static Pair source(long base, bool flag) {
  return Pair(Inner(base, base + 1, base + 2, base + 3), flag);
}

// The returned expression is a call, but its type is Pair and this returns
// Outer, so the conversion runs on the way out.
static Outer converted(long base, bool flag) { return source(base, flag); }

// Here the call's type is the return type, so eliding into the return slot is
// correct and must keep working.
static Pair forwarded(long base, bool flag) { return source(base, flag); }

int main() {
  Outer o = converted(10, true);
  if (o.a != 10) return 1;
  if (o.b != 11) return 2;
  if (o.c != 12) return 3;
  if (o.d != 13) return 4;
  if (!o.flag) return 5;

  Outer o2 = converted(20, false);
  if (o2.a != 20) return 6;
  if (o2.d != 23) return 7;
  if (o2.flag) return 8;

  Pair p = forwarded(30, true);
  if (p.first.a != 30) return 9;
  if (p.first.d != 33) return 10;
  if (!p.second) return 11;

  return 0;
}
