// RUN: -std=c++23
// EXPECT: 'this' is only valid inside a C++ member function

struct value {
  int invalid(this value& self) {
    return this == &self;
  }
};
