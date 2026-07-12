// RUN: -std=c++20
// EXPECT: constinit variable initializer is not a constant expression

int runtime_value();

int function() {
  static constinit int value = runtime_value();
  return value;
}
