// RUN: -std=c++26 -fcontracts=observe
// EXPECT_EXIT: 0

#include <contracts>

static int violations;
static int metadata_error;

void handle_contract_violation(
    const std::contracts::contract_violation& violation) {
  ++violations;
  if (violation.kind() != std::contracts::assertion_kind::assert ||
      violation.semantic() !=
          std::contracts::evaluation_semantic::observe ||
      violation.detection_mode() !=
          std::contracts::detection_mode::predicate_false ||
      violation.is_terminating() ||
      violation.comment()[0] != 'c' ||
      violation.location().line() == 0 ||
      violation.location().file_name()[0] == '\0' ||
      violation.location().function_name()[0] != 'm') {
    metadata_error = 1;
  }
}

int main() {
  contract_assert (false);
  return violations == 1 && metadata_error == 0 ? 0 : 1;
}
