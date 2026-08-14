// RUN: -std=c++26 -fcontracts=ignore
// EXPECT_EXIT: 0

static int evaluations;

static bool predicate() {
  ++evaluations;
  return false;
}

int checked()
    pre (predicate())
    post (predicate()) {
  return 7;
}

int main() {
  contract_assert (predicate());
  return checked() == 7 && evaluations == 0 ? 0 : 1;
}
