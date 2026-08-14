// RUN: -std=c++26 -fcontracts=observe
// EXPECT_EXIT: 0

#include <contracts>

static long sequence;

static bool record(int digit, bool value) {
  sequence = sequence * 10 + digit;
  return value;
}

void handle_contract_violation(
    const std::contracts::contract_violation& violation) {
  sequence = sequence * 10 +
             (violation.kind() == std::contracts::assertion_kind::pre ? 7 : 9);
}

int ordered()
    pre (record(1, false))
    pre (record(2, true))
    post (record(4, false))
    post (record(5, true)) {
  sequence = sequence * 10 + 3;
  return 0;
}

int main() {
  ordered();
  return sequence == 1723495 ? 0 : 1;
}
