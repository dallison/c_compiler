// A member template's default template argument may expand a pack of the
// enclosing class template: `A` is visible while the member's own template
// parameter list is parsed.
#include <utility>

template <class... A> struct S {
  int v = 0;
  template <class C,
            class = decltype(std::declval<C&>()(std::declval<A>()...))>
  S(C&& c) : v(c(A()...)) {}
  template <class C, class = decltype(std::declval<C&>()(A()...))>
  static int call(C c) {
    return c(A(1)...);
  }
};

struct F {
  int operator()(int a, long b) { return a + (int)b + 7; }
};

int main() {
  S<int, long> s(F{});
  if (s.v != 7) return 1;
  if (S<int, long>::call(F{}) != 9) return 2;
  return 0;
}
