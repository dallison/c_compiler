// RUN: -std=c++20
// EXPECT: anonymous bit-field cannot have a default member initializer
struct S {
  int : 2 = 1;
};
