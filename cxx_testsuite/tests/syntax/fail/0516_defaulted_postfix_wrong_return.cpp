// RUN: -std=c++29
// EXPECT: a defaulted postfix operator must return its operand type

struct counter {
  counter& operator++();
  int operator++(int) = default;
};
