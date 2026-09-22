// RUN: -std=c++17
// EXPECT_EXIT: 0

// A function parameter of class type with an initializer-list constructor
// (including a SFINAE member-template ctor, as in Abseil Span) must be
// viable for a braced-init argument so `Streamable(fmt, {Arg(x)...})` ranks.

#include <cstddef>
#include <initializer_list>
#include <type_traits>

template <class T>
struct Span {
  T* ptr;
  size_t len;

  template <class U>
  using EnableIfConst =
      typename std::enable_if<std::is_const<T>::value, U>::type;

  Span() : ptr(nullptr), len(0) {}
  Span(T* p, size_t n) : ptr(p), len(n) {}

  template <class LazyT = T, class = EnableIfConst<LazyT>>
  Span(std::initializer_list<typename std::remove_const<T>::type> v)
      : Span(v.begin(), v.size()) {}
};

struct Arg {
  int v;
  explicit Arg(int x) : v(x) {}
};

struct Streamable {
  int n;
  int first;
  Streamable(int, Span<const Arg> args)
      : n((int)args.len), first(args.len ? args.ptr[0].v : -1) {}
};

int take_span(Span<const int> s) { return (int)s.len + s.ptr[0]; }

int main() {
  int a = take_span({6, 7});
  Arg x(4);
  Streamable s(1, {x});
  return (a == 8 && s.n == 1 && s.first == 4) ? 0 : 1;
}
