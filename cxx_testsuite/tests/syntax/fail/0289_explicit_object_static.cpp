// RUN: -std=c++23
// EXPECT: explicit object member function cannot be static

struct value {
  static void invalid(this value& self);
};
