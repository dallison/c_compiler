// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// Inlining moves each actual argument out of the call node, addressing it by the
// child_id stored on the argument.  Default arguments used to be numbered as if
// slot 0 belonged to the callee, so moving one cleared the *next* argument slot,
// and moving the last one wrote past the end of the argument array into an
// unrelated heap block.

__attribute__((always_inline)) static int TrailingDefault(int a, int b = 40) {
  return a + b;
}

__attribute__((always_inline)) static int TwoDefaults(int a, int b = 3,
                                                      int c = 500) {
  return a + b * 10 + c;
}

struct Counter {
  int value;

  __attribute__((always_inline)) int Bump(int by = 1, int scale = 2) {
    value += by * scale;
    return value;
  }
};

int main() {
  if (TrailingDefault(2) != 42) {
    return 1;
  }
  if (TrailingDefault(2, 5) != 7) {
    return 2;
  }
  if (TwoDefaults(1) != 531) {
    return 3;
  }
  if (TwoDefaults(1, 2) != 521) {
    return 4;
  }
  if (TwoDefaults(1, 2, 3) != 24) {
    return 5;
  }
  Counter counter{10};
  if (counter.Bump() != 12) {
    return 6;
  }
  if (counter.Bump(5) != 22) {
    return 7;
  }
  if (counter.Bump(5, 3) != 37) {
    return 8;
  }
  return 0;
}
