// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

struct Sum4 {
  template <class A, class B, class C, class D>
  long operator()(A a, B b, C c, D d) const {
    return (long)a + (long)b + (long)c + (long)d;
  }
};

int main(void) {
  std::variant<int, long> a(1);
  std::variant<int, long> b(2L);
  std::variant<int, long> c(3);
  std::variant<int, long> d(4L);

  if (std::visit(Sum4{}, a, b, c, d) != 10) {
    return 1;
  }

  a.emplace<1>(10L);
  c.emplace<1>(30L);
  if (std::visit(Sum4{}, a, b, c, d) != 46) {
    return 2;
  }

  return 0;
}
