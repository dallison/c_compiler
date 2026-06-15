// RUN: -std=c++17
// EXPECT: Constructors cannot be virtual
struct Bad {
  virtual Bad();
};
