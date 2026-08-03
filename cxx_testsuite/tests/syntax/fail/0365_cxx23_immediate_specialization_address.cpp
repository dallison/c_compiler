// RUN: -std=c++23
// EXPECT: immediate function may only be named

consteval int identity(int value) {
  return value;
}

template <typename T>
constexpr int twice(T value) {
  return value + identity(value);
}

auto invalid = &twice<int>;
