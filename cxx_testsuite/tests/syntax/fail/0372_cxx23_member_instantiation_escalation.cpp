// RUN: -std=c++23
// EXPECT: consteval function call is not a constant expression

consteval int identity(int value) {
  return value;
}

template <typename T>
struct wrapper {
  constexpr int apply(T value) {
    return identity(value);
  }
};

int runtime(wrapper<int>& object, int value) {
  return object.apply(value);
}
