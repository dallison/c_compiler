// RUN: -std=c++29
// EXPECT: base class is initialized by both positional and designated initializers

struct A {
  int first;
  int second;
};

struct B : A {
  int member;
};

B object{A{1, 2}, .second = 3, .member = 4};
