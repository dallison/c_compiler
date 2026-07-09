// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

struct Sum5 {
  template <class A, class B, class C, class D, class E>
  long operator()(A a, B b, C c, D d, E e) const {
    return (long)a + (long)b + (long)c + (long)d + (long)e;
  }
};

int main(void) {
  std::variant<int, long> a(1);
  std::variant<int, long> b(2L);
  std::variant<int, long> c(3);
  std::variant<int, long> d(4L);
  std::variant<int, long> e(5);

  if (std::visit(Sum5{}, a, b, c, d, e) != 15) {
    return 1;
  }

  b.emplace<0>(20);
  d.emplace<0>(40);
  if (std::visit(Sum5{}, a, b, c, d, e) != 69) {
    return 2;
  }

  return 0;
}
