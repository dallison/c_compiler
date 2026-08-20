// RUN: -std=c++20 -fconstexpr-eval=audit

constexpr int& identity(int& value) {
  return value;
}

constexpr bool mutate_through_returned_reference() {
  int value = 1;
  identity(value) = 2;
  identity(value) += 3;
  ++identity(value);
  return value == 6;
}

static_assert(mutate_through_returned_reference());
