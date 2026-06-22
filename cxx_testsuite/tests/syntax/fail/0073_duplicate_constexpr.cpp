// RUN: -std=c++20

constexpr constexpr int duplicate_constexpr(void) {
  return 42;
}

static_assert(duplicate_constexpr() == 42,
              "duplicate constexpr specifiers are invalid");
