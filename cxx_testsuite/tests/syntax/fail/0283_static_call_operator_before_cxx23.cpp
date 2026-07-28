// RUN: -std=c++20
// EXPECT: static operator() and operator[] require C++23

struct callable {
  static int operator()(int value) {
    return value;
  }
};
