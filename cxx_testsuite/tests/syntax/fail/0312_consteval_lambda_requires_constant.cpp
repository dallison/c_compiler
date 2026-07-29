// RUN: -std=c++20
// EXPECT: consteval function call is not a constant expression

int runtime_value();

int call_immediate_lambda_at_runtime() {
  return [](int value) consteval { return value + 1; }(runtime_value());
}
