// RUN: -std=c++26
// EXPECT: returned reference cannot be initialized with a temporary expression

const double& converted() {
  static int value = 42;
  return value;
}
