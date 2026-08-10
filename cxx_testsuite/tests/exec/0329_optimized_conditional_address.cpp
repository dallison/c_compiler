// RUN: -std=c++23 -O1
// EXPECT_EXIT: 0

static int use_conditional_address(bool use_local, int* external) {
  int local = 11;
  int* selected = use_local ? &local : external;
  *selected += 7;
  return use_local ? local : *external;
}

int main() {
  int external = 19;
  if (use_conditional_address(true, &external) != 18) return 1;
  if (external != 19) return 2;
  if (use_conditional_address(false, &external) != 26) return 3;
  if (external != 26) return 4;
  return 0;
}
