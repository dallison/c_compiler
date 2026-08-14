// RUN: -std=c++26 -fcontracts=enforce
// EXPECT_EXIT: 0

#include <contracts>

static int seen;

void handle_contract_violation(
    const std::contracts::contract_violation& violation) {
  if (violation.is_terminating() &&
      violation.semantic() ==
          std::contracts::evaluation_semantic::enforce) {
    ++seen;
  }
  throw 17;
}

int main() {
  try {
    contract_assert (false);
  } catch (int value) {
    return seen == 1 && value == 17 ? 0 : 1;
  }
  return 2;
}
