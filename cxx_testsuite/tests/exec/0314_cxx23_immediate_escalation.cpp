// RUN: -std=c++23
// EXPECT_EXIT: 0

consteval int identity(int value) {
  return value;
}

template <typename T>
constexpr int twice(T value) {
  if constexpr (sizeof(T) == sizeof(int)) {
    return value + identity(value);
  } else {
    return value + value;
  }
}

template <typename T>
constexpr int guarded(T value) {
  if consteval {
    return identity(value);
  } else {
    return value;
  }
}

static_assert(twice(3) == 6);

int main() {
  volatile char value = 4;
  if (twice(value) != 8) {
    return 1;
  }
  if (guarded(9) != 9) {
    return 2;
  }
  return 0;
}
