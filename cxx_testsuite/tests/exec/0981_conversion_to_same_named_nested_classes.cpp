// RUN: -std=c++20
// EXPECT_EXIT: 0
// Conversion functions to different classes that share a name are different
// functions; the class each is nested in tells them apart.

struct A {
  struct T {
    int v;
  };
};
struct B {
  struct T {
    int v;
  };
};

template <class U>
struct Outer {
  template <class V>
  struct Inner {
    int v;
  };
};

struct X {
  operator A::T() const { return {1}; }
  operator B::T() const { return {2}; }
  operator Outer<int>::Inner<char>() const { return {3}; }
  operator Outer<long>::Inner<char>() const { return {4}; }
};

int main() {
  X x;
  A::T a = x;
  B::T b = x;
  Outer<int>::Inner<char> i = x;
  Outer<long>::Inner<char> l = x;
  if (a.v != 1 || b.v != 2 || i.v != 3 || l.v != 4) {
    return 1;
  }
  return 0;
}
