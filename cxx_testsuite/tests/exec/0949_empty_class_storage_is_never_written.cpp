// RUN: -std=c++20
// EXPECT_EXIT: 0

// An empty class -- including a captureless lambda's closure type -- is
// is_empty, and its one byte of storage may overlap other data as an empty
// base or a [[no_unique_address]] member ([intro.object]/8).  Copying or
// assigning such an object must therefore leave the overlapped bytes alone
// (abseil's CompressedTuple, protobuf's MemoizeProjection static_assert).
#include <new>
#include <type_traits>

struct E {};

template <class F>
struct W : F {
  int x;
  W(F f, int v) : F(f), x(v) {}
};

template <class F>
struct S {
  int x;
  [[no_unique_address]] F f;
};

struct M {
  [[no_unique_address]] E e;
  int x;
};

struct N {
  int x;
  [[no_unique_address]] E e;
};

int main() {
  auto twice = [](int v) { return v * 2; };
  auto generic = [](auto* p) { return p; };
  using L = decltype(twice);
  static_assert(std::is_empty_v<L>);
  static_assert(std::is_empty_v<decltype(generic)>);
  static_assert(sizeof(L) == 1);
  static_assert(sizeof(M) == sizeof(int));
  static_assert(sizeof(N) == sizeof(int));
  static_assert(sizeof(W<L>) == sizeof(int));
  static_assert(sizeof(S<L>) == sizeof(int));

  E e;
  W<E> a(e, 0x55667788);
  ::new (static_cast<E*>(&a)) E(e);
  if (a.x != 0x55667788) return 1;
  static_cast<E&>(a) = e;
  if (a.x != 0x55667788) return 2;

  W<L> b(twice, 0x11223344);
  ::new (static_cast<L*>(&b)) L(twice);
  if (b.x != 0x11223344) return 3;
  static_cast<L&>(b) = twice;
  if (b.x != 0x11223344 || b(4) != 8) return 4;

  S<L> s{0x01020304, twice};
  if (s.x != 0x01020304 || s.f(5) != 10) return 5;
  s.f = twice;
  if (s.x != 0x01020304) return 6;

  N n1{0x0a0b0c0d, {}};
  N n2{0x01010101, {}};
  n2.e = n1.e;
  if (n2.x != 0x01010101) return 7;
  n2 = n1;
  if (n2.x != 0x0a0b0c0d) return 8;

  int k = 3;
  auto capturing = [k](int v) { return v + k; };
  static_assert(!std::is_empty_v<decltype(capturing)>);
  if (capturing(1) != 4) return 9;
  return 0;
}
