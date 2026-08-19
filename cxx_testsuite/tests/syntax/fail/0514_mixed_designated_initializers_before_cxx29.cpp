// RUN: -std=c++26
// EXPECT: mixing designated and non-designated initializers requires C++29

struct A {
  int a;
};

struct B : A {
  int b;
};

B object{{.a = 1}, .b = 2};
