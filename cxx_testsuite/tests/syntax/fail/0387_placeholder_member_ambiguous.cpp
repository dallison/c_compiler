// RUN: -std=c++26
// EXPECT: name-independent declaration '_' is ambiguous

struct pair {
  int _;
  int _;
};

int read(pair& value) {
  return value._;
}
