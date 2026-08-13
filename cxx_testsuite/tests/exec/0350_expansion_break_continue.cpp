// RUN: -std=c++26
// EXPECT_EXIT: 0
static int test_break(void) {
  int count = 0;
  template for (auto x : {1, 2, 3, 4}) {
    if (x == 3) {
      break;
    }
    count++;
  }
  return count == 2 ? 0 : 1;
}

static int test_continue(void) {
  int count = 0;
  template for (auto x : {1, 2, 3, 4}) {
    if (x == 2) {
      continue;
    }
    count++;
  }
  return count == 3 ? 0 : 1;
}

int main(void) {
  return test_break() | test_continue();
}
