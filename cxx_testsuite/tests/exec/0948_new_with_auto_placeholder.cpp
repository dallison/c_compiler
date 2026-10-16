// RUN: -std=c++20
// EXPECT_EXIT: 0

// [expr.new]/2: a new-type-id containing `auto` is deduced from the single
// initializer as for `auto x(e);` -- arrays and functions decay and top-level
// cv is dropped -- including inside templates, where the initializer may be
// type-dependent.
#include <string>
#include <type_traits>

struct Agg {
  int a;
  double b;
};

struct Counted {
  int v;
  static int copies;
  explicit Counted(int x) : v(x) {}
  Counted(const Counted& o) : v(o.v + 100) { ++copies; }
};
int Counted::copies = 0;

template <class T>
auto Wrap(const T& t) { return new auto(t); }

template <class T>
auto Doubled(T t) { return new const auto{t * 2}; }

template <class T>
T* Dup(T v) { return new auto(v); }

int main() {
  int arr[3] = {1, 2, 3};
  auto* p1 = new auto(arr);
  static_assert(std::is_same_v<decltype(p1), int**>);
  if ((*p1)[2] != 3) return 1;

  auto p2 = new auto{7};
  static_assert(std::is_same_v<decltype(p2), int*>);
  if (*p2 != 7) return 2;

  const int ci = 9;
  auto p3 = new const auto(ci);
  static_assert(std::is_same_v<decltype(p3), const int*>);
  auto p4 = new auto(ci);
  static_assert(std::is_same_v<decltype(p4), int*>);
  if (*p3 != 9 || *p4 != 9) return 3;

  int x = 5;
  auto p5 = new auto*(&x);
  static_assert(std::is_same_v<decltype(p5), int**>);
  if (**p5 != 5) return 4;

  Agg a{1, 2.5};
  auto p6 = new auto(a);
  if (p6->a != 1 || p6->b != 2.5) return 5;

  Counted c(3);
  auto p7 = new auto(c);
  if (p7->v != 103 || Counted::copies != 1) return 6;

  auto p8 = new auto(std::string("hello"));
  if (*p8 != "hello") return 7;

  auto p9 = Wrap(std::string("w"));
  static_assert(std::is_same_v<decltype(p9), std::string*>);
  if (*p9 != "w") return 8;

  auto p10 = Doubled(2.5);
  static_assert(std::is_same_v<decltype(p10), const double*>);
  if (*p10 != 5.0) return 9;

  auto lambda = [](auto v) { return new auto(v + 1); };
  auto p11 = lambda(41L);
  static_assert(std::is_same_v<decltype(p11), long*>);
  if (*p11 != 42) return 10;

  int* p12 = Dup(4);
  if (*p12 != 4) return 11;

  delete p1; delete p2; delete p3; delete p4; delete p5; delete p6;
  delete p7; delete p8; delete p9; delete p10; delete p11; delete p12;
  return 0;
}
