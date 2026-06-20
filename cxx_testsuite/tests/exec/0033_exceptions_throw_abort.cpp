// RUN: -std=c++20
// EXPECT_EXIT: nonzero

int main(void) {
  throw 42;
  return 1;
}
