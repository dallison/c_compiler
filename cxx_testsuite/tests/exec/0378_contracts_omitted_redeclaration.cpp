// RUN: -std=c++26 -fcontracts=observe
// EXPECT_EXIT: 0

#include <contracts>

static int violations;

void handle_contract_violation(
    const std::contracts::contract_violation& violation) {
  if (violation.kind() == std::contracts::assertion_kind::pre) {
    ++violations;
  }
}

int declared_contract() pre (false);

int declared_contract() {
  return 42;
}

int main() {
  return declared_contract() == 42 && violations == 1 ? 0 : 1;
}
