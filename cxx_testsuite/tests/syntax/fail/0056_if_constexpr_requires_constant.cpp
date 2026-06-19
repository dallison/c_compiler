// RUN: -std=c++20
// EXPECT: if constexpr condition is not a constant expression
int runtime(void);

int value(void) {
  if constexpr (runtime()) {
    return 1;
  } else {
    return 2;
  }
}
