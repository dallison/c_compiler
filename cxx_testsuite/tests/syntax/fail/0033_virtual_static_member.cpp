// RUN: -std=c++17
// EXPECT: static member functions cannot be virtual
struct Bad {
  virtual static int value(void);
};
