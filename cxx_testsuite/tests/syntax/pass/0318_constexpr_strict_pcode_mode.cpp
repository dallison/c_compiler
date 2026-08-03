// RUN: -std=c++20 -fconstexpr-eval=pcode

constexpr int twice(int value) {
  return value * 2;
}

struct strict_point {
  int x;
  int y;

  constexpr strict_point(int x_value, int y_value)
      : x(x_value), y(y_value) {}
};

constexpr strict_point point(20, 22);

constexpr int allocated_value() {
  int* value = new int(42);
  int result = *value;
  delete value;
  return result;
}

static_assert(twice(21) == 42);
static_assert(point.x + point.y == 42);
static_assert(allocated_value() == 42);
