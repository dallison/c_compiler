// RUN: -std=c++20
// EXPECT: Plain is not a class template

struct Plain {
  int value;
};

template struct Plain;
