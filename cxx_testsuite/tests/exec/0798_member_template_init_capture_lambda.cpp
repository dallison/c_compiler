// RUN: -std=c++17
// EXPECT_EXIT: 0

// Init-captures in a member function template of a class template name the
// member's own parameters, so the closure fields can only be deduced per call.

struct Fn {
  template <typename F>
  Fn(F f) : value(f()) {}
  int value;
};

template <bool owned>
struct V {
  template <typename Cb>
  int Local(Cb cb) {
    auto l = [c = cb]() mutable { return c(); };
    return l();
  }

  template <typename Cb>
  int Copied(Cb cb) {
    Cb d = cb;
    auto l = [c = d, k = 2]() mutable { return c() * k; };
    return l();
  }

  template <typename Cb>
  Fn Wrapped(Cb cb) {
    return Fn([c = cb]() mutable { return c() + (owned ? 1 : 0); });
  }
};

template <typename T>
struct Box {
  T base;
  template <typename Cb>
  T Apply(Cb cb) {
    auto l = [c = cb, b = base]() { return c(b); };
    return l();
  }
};

int main() {
  V<true> v;
  if (v.Local([] { return 3; }) != 3) {
    return 1;
  }
  if (v.Local([] { return 4L; }) != 4) {
    return 2;
  }
  if (v.Copied([] { return 5; }) != 10) {
    return 3;
  }
  if (v.Wrapped([] { return 6; }).value != 7) {
    return 4;
  }
  V<false> w;
  if (w.Wrapped([] { return 6; }).value != 6) {
    return 5;
  }
  Box<int> box{8};
  if (box.Apply([](int x) { return x * 3; }) != 24) {
    return 6;
  }
  return 0;
}
