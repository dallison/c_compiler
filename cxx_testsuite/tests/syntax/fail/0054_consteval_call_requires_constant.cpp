// RUN: -std=c++20
int runtime(void);

consteval int immediate(int value) {
  return value + 1;
}

int value = immediate(runtime());
// EXPECT: consteval function call is not a constant expression
