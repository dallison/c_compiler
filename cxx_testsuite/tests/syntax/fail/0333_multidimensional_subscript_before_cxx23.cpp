// RUN: -std=c++20
// EXPECT: operator[] requires exactly one parameter before C++23

struct grid {
  int operator[](int row, int column) const;
};
