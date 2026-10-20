// RUN: -std=c++20
// EXPECT_EXIT: 0

struct S {
  template <class T>
  int g();
  template <int N>
  int k();
};

template <>
int S::g<int>() {
  return 1;
}

template <>
int S::g<char>() {
  return 2;
}

template <>
int S::k<1>() {
  return 3;
}

template <>
int S::k<2>() {
  return 4;
}

template <class T>
struct A {
  int f() { return 0; }
  template <class U>
  int h() {
    return 0;
  }
};

template <>
int A<int>::f() {
  return 5;
}

template <>
int A<char>::f() {
  return 6;
}

template <>
template <>
int A<int>::h<int>() {
  return 7;
}

template <>
template <>
int A<char>::h<int>() {
  return 8;
}

template <>
template <>
int A<int>::h<char>() {
  return 9;
}

int main() {
  S s;
  if (s.g<int>() != 1) return 1;
  if (s.g<char>() != 2) return 2;
  if (s.k<1>() != 3) return 3;
  if (s.k<2>() != 4) return 4;
  A<int> a;
  A<char> b;
  A<long> c;
  if (a.f() != 5) return 5;
  if (b.f() != 6) return 6;
  if (c.f() != 0) return 7;
  if (a.h<int>() != 7) return 8;
  if (b.h<int>() != 8) return 9;
  if (a.h<char>() != 9) return 10;
  if (c.h<int>() != 0) return 11;
  return 0;
}
