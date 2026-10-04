// RUN: -std=c++20
// EXPECT_EXIT: 0

// A named return value whose alignment exceeds the stack alignment (an
// `alignas(64)` class, or one holding a 16-byte `long double`) cannot live in
// the caller's return slot; the return must still copy it out.
struct A {
  long long s;
  long double d;
  A() : s(0), d(0) {}
};

struct Store {
  A v[1];
  unsigned long count;
  Store() : count(0) {}
};

Store make(int x) {
  Store s;
  s.v[0].s = x;
  ++s.count;
  return s;
}

struct alignas(64) Wide {
  unsigned long count;
  Wide() : count(0) {}
};

Wide make_wide(int x) {
  Wide w;
  w.count += x;
  return w;
}

int main() {
  Store st = make(56);
  if (st.count != 1) return 10;
  if (st.v[0].s != 56) return 11;
  Wide w = make_wide(3);
  if (w.count != 3) return 12;
  return 0;
}
