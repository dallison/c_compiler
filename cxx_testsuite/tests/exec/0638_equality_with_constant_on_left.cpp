// RUN: -std=c++20
// EXPECT_EXIT: 0

// `0 == x` and `0 != x` are commuted to put the constant on the right; the
// equality conditions must stay the same when the operands are swapped.
typedef unsigned long size_t;

__attribute__((noinline)) int f1(size_t idx) {
  size_t c = 0;
  int r = 9;
  (c++ == idx ? r = 2 : r);
  return r + (int)c * 0;
}
__attribute__((noinline)) int f2(size_t idx) {
  int r = 9;
  if (0 == idx) r = 2;
  return r;
}
__attribute__((noinline)) int f3(size_t idx) {
  size_t c = 0;
  int r = 9;
  if (c++ == idx) r = 2;
  return r;
}
__attribute__((noinline)) int f4(size_t idx) {
  int r = 9;
  (0 == idx ? r = 2 : r);
  return r;
}
__attribute__((noinline)) int f5(size_t idx) {
  int r = 9;
  (0 != idx ? r : (r = 2));
  return r;
}
__attribute__((noinline)) int f6(int idx) {
  return (7 == idx) * 10 + (7 != idx);
}

int main() {
  if (f1(0) != 2 || f1(1) != 9) return 1;
  if (f2(0) != 2 || f2(1) != 9) return 2;
  if (f3(0) != 2 || f3(1) != 9) return 3;
  if (f4(0) != 2 || f4(1) != 9) return 4;
  if (f5(0) != 2 || f5(1) != 9) return 5;
  if (f6(7) != 10 || f6(8) != 1) return 6;
  return 0;
}
