// RUN: -std=c++17
// EXPECT_EXIT: 0
// Array bounds kept as dependent expressions (`char[sizeof(U)]`,
// `char[Sz<U>()]`, a constexpr local) are substituted when the template is
// instantiated, in free function templates and in member templates of a class
// template, as template arguments, in sizeof, in aliases and in local arrays.
template <class K> int FreeGet() { return (int)sizeof(K); }
template <class U> constexpr int Sz() { return sizeof(U); }

template <typename X> struct A {
  // Instantiated before the free F3 below, which names the same `Sz<U>`.
  template <typename U> int Off3() const { return FreeGet<char[Sz<U>()]>(); }
  template <typename U> int Off1() const { return FreeGet<char[sizeof(U)]>(); }
  template <typename U> int Off2() const { return sizeof(char[sizeof(U)]); }
  template <typename U> int Off4() const {
    constexpr int i = sizeof(U);
    return FreeGet<char[i]>();
  }
  template <typename U> int Off5() const {
    using T = char[sizeof(U)];
    return sizeof(T);
  }
  template <typename U> int Off6() const {
    return FreeGet<char[sizeof(X) + sizeof(U)]>();
  }
};

template <typename U> int F1() { return FreeGet<char[sizeof(U)]>(); }
template <typename U> int F2() { return sizeof(char[sizeof(U)]); }
template <typename U> int F3() { return FreeGet<char[Sz<U>()]>(); }
template <typename U> int F4() {
  constexpr int i = sizeof(U);
  return FreeGet<char[i]>();
}
template <typename U> int F5() {
  using T = char[sizeof(U)];
  return sizeof(T);
}
template <typename U> int F6() {
  char buf[sizeof(U)];
  return sizeof(buf);
}
template <typename U> int F7() {
  using T = char[Sz<U>()];
  return sizeof(T);
}
template <int N> int F8() { return sizeof(char[N + 1]); }

template <class L, class R> struct Same { static constexpr bool value = false; };
template <class L> struct Same<L, L> { static constexpr bool value = true; };
template <typename T> constexpr int Find() { return -1; }
template <typename T, typename T1, typename... Ts> constexpr int Find() {
  return Same<T, T1>::value ? 0 : Find<T, Ts...>() + 1;
}
// The pack overload is the only viable Find with three arguments.
template <typename X> struct Alloc {
  template <typename U> int Off() const {
    return FreeGet<char[Find<U, char, X>() + 1]>();
  }
};
char buf[Find<int, char, int>() + 1];
template <class U> int G() { return FreeGet<char[Find<U, char, int>() + 1]>(); }

int main() {
  A<int> a;
  if (a.Off3<char>() != 1 || a.Off3<int>() != 4) return 1;
  if (a.Off1<char>() != 1 || a.Off1<int>() != 4) return 2;
  if (a.Off2<char>() != 1 || a.Off2<int>() != 4) return 3;
  if (a.Off4<char>() != 1 || a.Off4<int>() != 4) return 4;
  if (a.Off5<char>() != 1 || a.Off5<int>() != 4) return 5;
  if (a.Off6<char>() != 5) return 6;
  if (F1<short>() != 2 || F2<short>() != 2 || F3<short>() != 2) return 7;
  if (F4<short>() != 2 || F5<short>() != 2 || F6<short>() != 2) return 8;
  if (F7<short>() != 2 || F8<1>() != 2) return 9;
  if (F3<int>() != 4) return 10;
  Alloc<int> alloc;
  if (alloc.Off<char>() != 1 || alloc.Off<int>() != 2) return 11;
  if (sizeof(buf) != 2 || G<int>() != 2) return 12;
  return 0;
}
