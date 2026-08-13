// RUN: -std=c++26
// EXPECT: name-independent declaration '_' is ambiguous

struct pair {
  int _;
  long _;
};

pair value{._ = 1};
