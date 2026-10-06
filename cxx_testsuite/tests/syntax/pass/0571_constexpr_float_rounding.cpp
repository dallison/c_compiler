// RUN: -std=c++20 -fconstexpr-eval=audit

// A constant expression of type float has a float value: literals, arithmetic
// and results round to float precision rather than keeping double precision.
constexpr float largest() { return 3.40282347e+38F; }
static_assert(largest() > 1.0f);

constexpr float sum() {
  float a = 0.1f;
  float b = 0.2f;
  return a + b;
}
static_assert(sum() == 0.3f);

constexpr double widened_sum() { return 0.1f + 0.2f; }
static_assert(widened_sum() == static_cast<double>(0.1f + 0.2f));

constexpr float compounded() {
  float x = 1.0f;
  for (int i = 0; i < 10; i++) {
    x = x * 1.1f;
  }
  return x;
}
static_assert(compounded() > 2.5f);
