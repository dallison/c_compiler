// RUN: -std=c++23
// EXPECT: consteval function call is not a constant expression

consteval int identity(int value) {
  return value;
}

template <typename T>
constexpr int twice(T value) {
  return value + identity(value);
}

int runtime(int value) {
  return twice(value);
}
