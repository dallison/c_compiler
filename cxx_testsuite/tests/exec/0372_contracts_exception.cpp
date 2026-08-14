// RUN: -std=c++26 -fcontracts=observe
// EXPECT_EXIT: 0

#include <contracts>

static int seen;

void handle_contract_violation(
    const std::contracts::contract_violation& violation) {
  if (violation.detection_mode() ==
      std::contracts::detection_mode::evaluation_exception) {
    ++seen;
  }
}

static bool throwing_predicate() {
  throw 42;
}

int main() {
  contract_assert (throwing_predicate());
  return seen == 1 ? 0 : 1;
}
