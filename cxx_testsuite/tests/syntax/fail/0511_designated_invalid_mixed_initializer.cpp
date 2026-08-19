// RUN: -std=c++29
// EXPECT: non-designated initializer in a mixed list must initialize a direct base class

struct A {
  int first;
  int second;
};

A object{1, .second = 2};
