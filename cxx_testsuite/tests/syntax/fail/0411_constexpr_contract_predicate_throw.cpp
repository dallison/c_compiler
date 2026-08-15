// RUN: -std=c++26 -fconstexpr-eval=audit
// EXPECT: static_assert expression is not an integer constant expression

constexpr bool throwing_predicate() {
  throw 1;
}

constexpr int checked() {
  contract_assert(throwing_predicate());
  return 42;
}

static_assert(checked() == 42);
