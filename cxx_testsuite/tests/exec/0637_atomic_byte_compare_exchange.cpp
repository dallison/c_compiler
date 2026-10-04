// RUN: -std=c++20
// EXPECT_EXIT: 0

// Sub-word compare-exchange: the expanded loop needs scratch registers that
// must not alias the pointer, expected value, or desired value.
static int test_and_set(bool* v) {
  bool expected = false;
  while (!__atomic_compare_exchange_n(v, &expected, true, false,
                                      __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)) {
    if (expected) return 1;
    expected = false;
  }
  return 0;
}

static int cas_short(unsigned short* v, unsigned short from,
                     unsigned short to) {
  unsigned short expected = from;
  if (__atomic_compare_exchange_n(v, &expected, to, false, __ATOMIC_SEQ_CST,
                                  __ATOMIC_SEQ_CST)) {
    return 0;
  }
  return expected;
}

int main() {
  bool flag = false;
  if (test_and_set(&flag)) return 1;
  if (!flag) return 2;
  if (!test_and_set(&flag)) return 3;
  unsigned short s = 0xfff0;
  if (cas_short(&s, 0xfff0, 0x8001) != 0) return 4;
  if (s != 0x8001) return 5;
  if (cas_short(&s, 0xfff0, 7) != 0x8001) return 6;
  if (s != 0x8001) return 7;
  return 0;
}
