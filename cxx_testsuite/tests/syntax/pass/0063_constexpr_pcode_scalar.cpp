// RUN: -std=c++20

constexpr int pcode_constant_int(void) {
  return 42;
}

constexpr double pcode_constant_double(void) {
  return 3.5;
}

static_assert(pcode_constant_int() == 42, "pcode constexpr integer scalar");
static_assert(pcode_constant_double() == 3.5, "pcode constexpr floating scalar");
