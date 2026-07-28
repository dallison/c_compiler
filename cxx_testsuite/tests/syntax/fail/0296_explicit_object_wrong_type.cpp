// RUN: -std=c++23
// EXPECT: explicit object parameter type must name the member's class or a derived class

struct value {
  void invalid(this int self);
};
