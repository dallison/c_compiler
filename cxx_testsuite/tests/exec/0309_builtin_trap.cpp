// RUN: -std=c++20
// EXPECT_EXIT: nonzero

int main(void) {
  __builtin_trap();
}
