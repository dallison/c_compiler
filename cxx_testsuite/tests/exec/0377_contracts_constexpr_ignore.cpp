// RUN: -std=c++26 -fcontracts=ignore
// EXPECT_EXIT: 0

constexpr int ignored_contract(const int value)
    pre (value > 0)
    post (result: result < 0) {
  contract_assert (false);
  return value;
}

static_assert(ignored_contract(0) == 0);

int main() {
  return ignored_contract(0);
}
