// RUN: -std=c++17
// EXPECT_EXIT: 0

// Mem-initializers of a member template constructor of a class template name
// the class's own nested types and members; they must resolve to the
// instantiation's, so the nested derived-to-base conversion to `Rank0` holds.

template <bool owned>
struct Value {
  struct Rank0 {};
  struct Rank1 : Rank0 {};
  struct Rank2 : Rank1 {};

  template <typename T>
  Value(T t) : value(Convert(t, Rank2{})), twice(Twice(t)) {}

  int value;
  int twice;

 private:
  static Rank2 Top() { return {}; }
  template <typename T>
  int Twice(T t) { return Convert(t, Top()) * 2; }
  int Convert(int t, Rank1) { return t + (owned ? 1 : 2); }
  int Convert(const char*, Rank0) { return 100; }
};

template <typename T>
struct Holder {
  template <typename U>
  Holder(U u) : inner(u, Tag{}) {}

  struct Tag {};
  struct Inner {
    Inner(int v, Tag) : v(v) {}
    int v;
  };
  Inner inner;
};

int main() {
  Value<true> a(5);
  Value<false> b(5);
  Value<true> c("x");
  Holder<long> h(9);
  if (a.value != 6 || a.twice != 12) return 1;
  if (b.value != 7) return 2;
  if (c.value != 100) return 3;
  return h.inner.v == 9 ? 0 : 4;
}
