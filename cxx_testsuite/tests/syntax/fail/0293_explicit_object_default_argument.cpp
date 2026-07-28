// RUN: -std=c++23
// EXPECT: explicit object parameter cannot have a default argument

struct value {
  void invalid(this value self = value{});
};
