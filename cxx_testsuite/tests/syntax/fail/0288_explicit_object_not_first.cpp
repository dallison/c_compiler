// RUN: -std=c++23
// EXPECT: explicit object parameter must be the first parameter

struct value {
  void invalid(int argument, this value& self);
};
