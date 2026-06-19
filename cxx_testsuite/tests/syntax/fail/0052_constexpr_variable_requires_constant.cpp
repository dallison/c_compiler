// RUN: -std=c++20
// EXPECT: constexpr variable initializer is not a constant expression
int runtime_value(void) {
  return 3;
}

constexpr int value = runtime_value();

int main(void) {
  return value;
}
