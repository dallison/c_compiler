// RUN: -std=c++20
// EXPECT: explicit object parameters require C++23

struct value {
  int read(this const value& self) {
    return 0;
  }
};
