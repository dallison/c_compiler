// RUN: -std=c++29
// EXPECT: designator order for field 'a' does not match declaration order in 'B'

struct A {
  int a;
};

struct B : A {
  int b;
};

B value{.b = 2, .a = 1};
