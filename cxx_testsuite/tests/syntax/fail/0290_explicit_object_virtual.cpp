// RUN: -std=c++23
// EXPECT: explicit object member function cannot be virtual

struct value {
  virtual void invalid(this value& self);
};
