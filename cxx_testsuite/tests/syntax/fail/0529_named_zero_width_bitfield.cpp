// RUN: -std=c++20
// EXPECT: named bit-field has zero width
struct S {
  int x : 0;
};
