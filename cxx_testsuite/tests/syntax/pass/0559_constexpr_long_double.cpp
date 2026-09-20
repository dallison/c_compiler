// RUN: -std=c++20 -fconstexpr-eval=pcode
constexpr long double add(long double a, long double b) {
  return a + b;
}

constexpr long double sub(long double a, long double b) {
  return a - b;
}

constexpr long double mul(long double a, long double b) {
  return a * b;
}

constexpr long double divl(long double a, long double b) {
  return a / b;
}

constexpr long double from_int(int value) {
  return (long double)value;
}

constexpr int to_int(long double value) {
  return (int)value;
}

static_assert(sizeof(long double) == 16, "p-code long double is 16 bytes");
static_assert(add(1.0L, 2.0L) == 3.0L, "constexpr long double add");
static_assert(sub(2.0L, 1.0L) == 1.0L, "constexpr long double sub");
static_assert(mul(2.0L, 2.0L) == 4.0L, "constexpr long double mul");
static_assert(divl(1.0L, 2.0L) == 0.5L, "constexpr long double div");
static_assert(-1.0L == 0.0L - 1.0L, "constexpr long double neg");
static_assert(from_int(42) == 42.0L, "constexpr long double from int");
static_assert(to_int(add(1.0L, 2.0L)) == 3, "constexpr long double to int");
// Narrower than binary64 epsilon, so this only holds with a distinct format.
static_assert(1.0L + 0x1p-60L > 1.0L, "constexpr long double extra precision");
