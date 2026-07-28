// RUN: -std=c++23
// EXPECT: constructors and destructors cannot have an explicit object parameter

struct value {
  value(this value& self);
};
