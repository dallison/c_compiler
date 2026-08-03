// RUN: -std=c++23 -fconstexpr-eval=audit

constexpr int cxx23_result(int value) {
  if (value > 0) {
    return value + 1;
  }
unused:
  return 0;
}

static_assert(cxx23_result(41) == 42);
