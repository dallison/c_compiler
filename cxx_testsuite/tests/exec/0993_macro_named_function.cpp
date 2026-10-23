// RUN: -std=c++17
// EXPECT_EXIT: 0

// The function name is also a function-like macro.  ARM unwind entries refer
// to the function; that reference must not be expanded as the macro.
int gzgetc(int value) { return value + 1; }
int (*saved_gzgetc)(int) = gzgetc;

#define gzgetc(value) ((value) + 2)

int main() {
  if (saved_gzgetc(1) != 2) return 1;
  if (gzgetc(1) != 3) return 2;
  return 0;
}
