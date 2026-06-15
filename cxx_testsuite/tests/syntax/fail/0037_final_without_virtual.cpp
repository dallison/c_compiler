// RUN: -std=c++17
// EXPECT: value marked final but is not virtual
struct Bad {
  int value(void) final;
};
