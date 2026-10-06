// RUN: -std=c++20 -fconstexpr-eval=audit

// Multiplying a narrow integer by a literal zero folds to zero, including
// when the result is then sign-extended or used in further arithmetic.
constexpr int times_zero(int n) { return n * 0 + 6; }
static_assert(times_zero(3) == 6);

constexpr int zero_times(int n) { return 0 * n; }
static_assert(zero_times(7) == 0);

constexpr short short_times_zero(short n) { return static_cast<short>(n * 0 - 1); }
static_assert(short_times_zero(5) == -1);

constexpr int f() {
  struct R {
    static constexpr int g(int n) {
      int v;
      v = n;
      return n == 0 ? 1 : g(n - 1) * v + 0 * v;
    }
  };
  return R::g(3) - 5;
}
static_assert(f() == 1);
