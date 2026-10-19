// RUN: -std=c++17
// EXPECT_EXIT: 0
// `Z<i>::type` with `i` a constexpr local computed from the member template's
// parameter is a different specialization per instantiation; the array bound
// `I + 1` in Z's member must see the substituted `i`, not the primary's `I`.
template <int N> struct Arr { char c[N + 1]; };
template <int I> struct Z { using type = char[I + 1]; };
template <int I> struct W { using type = Arr<I>; };
template <class U, class V> constexpr int One() { return sizeof(U) == 1 ? 0 : 1; }
template <class U> constexpr int Ov() { return 5; }
template <class U, class V, class T> constexpr int Ov() { return sizeof(U) == 1 ? 0 : 1; }
template <class K> int FreeGet() { return (int)sizeof(K); }

template <typename X> struct A {
  template <typename U> int Off() const {
    constexpr int i = One<U, char>();
    return FreeGet<typename Z<i>::type>() * 10 + i;
  }
  template <typename U> int Alias() const {
    constexpr int i = One<U, char>();
    using T = typename Z<i>::type;
    return sizeof(T);
  }
  template <typename U> int Class() const {
    constexpr int i = One<U, char>();
    return FreeGet<typename W<i>::type>();
  }
  template <typename U> int Direct() const {
    return FreeGet<typename Z<One<U, char>()>::type>();
  }
  template <typename U> int Overloaded() const {
    constexpr int i = Ov<U, char, short>();
    return FreeGet<typename Z<i>::type>();
  }
};

template <typename U> int Free() {
  constexpr int i = One<U, char>();
  return FreeGet<typename Z<i>::type>() * 10 + i;
}

int main() {
  A<int> a;
  if (a.Off<char>() != 10 || a.Off<int>() != 21) return 1;
  if (Free<char>() != 10 || Free<int>() != 21) return 2;
  if (a.Alias<char>() != 1 || a.Alias<int>() != 2) return 3;
  if (a.Class<char>() != 1 || a.Class<int>() != 2) return 4;
  if (a.Direct<char>() != 1 || a.Direct<int>() != 2) return 5;
  if (a.Overloaded<char>() != 1 || a.Overloaded<int>() != 2) return 6;
  return 0;
}
