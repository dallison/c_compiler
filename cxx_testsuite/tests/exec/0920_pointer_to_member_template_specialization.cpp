// `&S::Get<1>` and `&Q<int>::Get<2>` form pointers to member function
// template specializations, and `int (Q<int>::*)()` names the member pointer
// class Q<int>, not the primary template.
#include <cstddef>
#include <type_traits>

struct S {
  int v = 7;
  template <std::size_t J> int Get() { return v + (int)J; }
  int Twice() { return v * 2; }
  template <class T> int Twice() { return (int)sizeof(T); }
  template <class T> static T Make() { return T(3); }
};

template <class A> struct Q {
  A v = 5;
  template <std::size_t J> A Get() { return v + (A)J; }
};

int main() {
  int (S::*p)() = &S::Get<1>;
  int (Q<int>::*q)() = &Q<int>::Get<2>;
  long (Q<long>::*ql)() = &Q<long>::Get<4>;
  auto pa = &S::Get<3>;
  int (S::*plain)() = &S::Twice;
  int (S::*sized)() = &S::Twice<double>;
  long (*m)() = &S::Make<long>;
  static_assert(std::is_same<decltype(pa), int (S::*)()>::value, "");
  static_assert(std::is_same<decltype(&Q<int>::Get<0>), int (Q<int>::*)()>::value,
                "");

  S s;
  Q<int> qi;
  Q<long> qlong;
  if ((s.*p)() != 8) return 1;
  if ((qi.*q)() != 7) return 2;
  if ((qlong.*ql)() != 9) return 3;
  if ((s.*pa)() != 10) return 4;
  if ((s.*plain)() != 14) return 5;
  if ((s.*sized)() != (int)sizeof(double)) return 6;
  if (m() != 3) return 7;
  S* sp = &s;
  if ((sp->*p)() != 8) return 8;
  return 0;
}
