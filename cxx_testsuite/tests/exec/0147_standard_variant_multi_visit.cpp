// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

struct Add {
  template <class A, class B>
  long operator()(A a, B b) const {
    return (long)a + (long)b;
  }
};

struct Sum3 {
  template <class A, class B, class C>
  long operator()(A a, B b, C c) const {
    return (long)a + (long)b + (long)c;
  }
};

int main(void) {
  std::variant<int, long> a(3);
  std::variant<int, long> b(4L);
  if (std::visit(Add{}, a, b) != 7) {
    return 1;
  }

  a.emplace<1>(10L);
  if (std::visit(Add{}, a, b) != 14) {
    return 2;
  }

  const std::variant<int, long> ca(5);
  const std::variant<int, long> cb(6L);
  if (std::visit(Add{}, ca, cb) != 11) {
    return 3;
  }

  std::variant<int, long> c(7);
  if (std::visit(Sum3{}, a, b, c) != 21) {
    return 4;
  }

  long r = std::visit([](auto x, auto y) { return (long)x * (long)y; }, a, b);
  if (r != 40) {
    return 5;
  }

  // Single-variant visit must keep working.
  if (std::visit([](auto x) { return (long)x + 1; }, a) != 11) {
    return 6;
  }

  return 0;
}
