// RUN: -std=c++20
// EXPECT_EXIT: 0

// Leaving a catch handler by return, break, continue or a new exception ends
// the handler and destroys the caught exception object.
int live = 0;
struct S {
  int v;
  S(int x) : v(x) { ++live; }
  S(const S& o) : v(o.v) { ++live; }
  ~S() { --live; }
};

int ret() {
  try { throw S(1); } catch (const S& e) { return e.v; }
  return 0;
}
int brk(int n) {
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    try { throw S(i); } catch (const S& e) {
      sum += e.v;
      if (e.v == 2) break;
    }
  }
  return sum;
}
int cont(int n) {
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    try { throw S(i); } catch (const S& e) {
      if (e.v % 2) continue;
      sum += e.v;
    }
  }
  return sum;
}
int nested() {
  try {
    try { throw S(1); } catch (const S& inner) { throw S(inner.v + 1); }
  } catch (const S& outer) {
    return outer.v;
  }
  return 0;
}
int rethrown() {
  try {
    try { throw S(4); } catch (...) { throw; }
  } catch (const S& e) {
    return e.v;
  }
  return 0;
}

int main() {
  if (ret() != 1 || live != 0) return 1;
  if (brk(10) != 3 || live != 0) return 2;
  if (cont(6) != 6 || live != 0) return 3;
  if (nested() != 2 || live != 0) return 4;
  if (rethrown() != 4 || live != 0) return 5;
  return 0;
}
