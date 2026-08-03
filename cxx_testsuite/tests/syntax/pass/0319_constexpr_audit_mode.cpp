// RUN: -std=c++20 -fconstexpr-eval=audit

constexpr int audit_arithmetic(int left, int right) {
  return left * 10 + right;
}

constexpr double audit_floating(double value) {
  return value * 0.5;
}

static_assert(audit_arithmetic(4, 2) == 42);
static_assert(audit_floating(8.0) == 4.0);
