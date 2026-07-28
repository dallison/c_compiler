// RUN: -std=c++23
// EXPECT: Duplicate struct/union member operator()

struct callable {
  int operator()(int value) const {
    return value;
  }

  static int operator()(int value) {
    return value + 1;
  }
};
