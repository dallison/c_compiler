// RUN: -std=c++17
// EXPECT_EXIT: 0

int main() {
  int value = 2;
  int expected = 2;
  if (!__atomic_compare_exchange_n(&value, &expected, 1, 1, __ATOMIC_RELEASE,
                                   __ATOMIC_ACQUIRE)) {
    return 1;
  }
  if (value != 1) return 2;
  return 0;
}
