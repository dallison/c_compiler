// RUN: -std=c++20
// EXPECT: consteval function call is not a constant expression

consteval int identity(int value) {
  return value;
}

template <typename T>
constexpr int twice(T value) {
  return value + identity(value);
}

static_assert(twice(3) == 6);
