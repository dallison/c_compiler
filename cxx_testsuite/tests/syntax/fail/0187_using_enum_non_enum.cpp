// RUN: -std=c++20
// EXPECT: is not an enumeration type
struct S {
  int x;
};

using enum S;
