// RUN: -std=c++26 -fcontracts=quick-enforce
// EXPECT_EXIT: 134

#include <contracts>
#include <cstdlib>

void handle_contract_violation(
    const std::contracts::contract_violation&) {
  exit(5);
}

int main() {
  contract_assert (false);
  return 0;
}
