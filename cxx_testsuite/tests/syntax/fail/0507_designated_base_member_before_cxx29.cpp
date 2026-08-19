// RUN: -std=c++26
// EXPECT: designating an inherited member requires C++29

struct A {
  int a;
};

struct B : A {
  int b;
};

B value{.a = 1, .b = 2};
