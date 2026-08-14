// RUN: -std=c++26 -fcontracts=observe
// EXPECT_EXIT: 0

static int post_evaluations;

static bool postcondition() {
  ++post_evaluations;
  return true;
}

int throwing()
    post (postcondition()) {
  throw 1;
}

int main() {
  try {
    throwing();
  } catch (int) {
    return post_evaluations == 0 ? 0 : 1;
  }
  return 2;
}
