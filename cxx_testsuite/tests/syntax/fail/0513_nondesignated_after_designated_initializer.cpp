// RUN: -std=c++29
// EXPECT: non-designated initializer cannot follow a designated initializer

struct A {
  int a;
};

struct B : A {
  int b;
};

B object{.a = 1, A{2}};
