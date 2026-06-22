// RUN: -std=c++20

constexpr int pcode_constant_int(void) {
  return 42;
}

constexpr double pcode_constant_double(void) {
  return 3.5;
}

constexpr int pcode_integer_expression(void) {
  return ((6 * 7) + 10) / 2 - 5;
}

constexpr double pcode_floating_expression(void) {
  return (1.25 + 2.75) * 2.0;
}

constexpr int pcode_integer_parameters(int x, int y) {
  return x * 10 + y;
}

constexpr double pcode_floating_parameters(double x, double y) {
  return x / 2.0 + y;
}

struct PCodePair {
  int x;
  double y;
};

constexpr PCodePair pcode_pair = {
  pcode_integer_parameters(4, 2),
  pcode_floating_parameters(7.0, 0.5),
};

constexpr int pcode_array[3] = {
  1 + 1,
  pcode_integer_parameters(1, 2),
  5,
};

static_assert(pcode_constant_int() == 42, "pcode constexpr integer scalar");
static_assert(pcode_constant_double() == 3.5, "pcode constexpr floating scalar");
static_assert(pcode_integer_expression() == 21, "pcode constexpr integer expression");
static_assert(pcode_floating_expression() == 8.0, "pcode constexpr floating expression");
static_assert(pcode_integer_parameters(4, 2) == 42, "pcode constexpr integer parameters");
static_assert(pcode_floating_parameters(7.0, 0.5) == 4.0, "pcode constexpr floating parameters");
static_assert(pcode_pair.x == 42, "pcode constexpr object integer field");
static_assert(pcode_pair.y == 4.0, "pcode constexpr object floating field");
static_assert(pcode_array[1] == 12, "pcode constexpr array field");
