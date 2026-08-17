#include <stdatomic.h>

static atomic_int value = ATOMIC_VAR_INIT(1);
static atomic_flag flag = ATOMIC_FLAG_INIT;
static int values[4];
static _Atomic(int*) pointer = values;
static _Atomic int slots[2];
static _Atomic float fraction = 1.25f;
static struct {
  _Atomic int member;
} holder = {1};

int main(void) {
  if (fraction != 1.25f) {
    return 14;
  }
  fraction = 2.5f;
  if (fraction != 2.5f) {
    return 15;
  }
  if (value != 1) {
    return 1;
  }
  value = 2;
  atomic_store_explicit(&value, 2, memory_order_relaxed);
  if (atomic_load_explicit(&value, memory_order_relaxed) != 2 ||
      value++ != 2 || value != 3) {
    return 2;
  }
  if (++value != 4) {
    return 3;
  }
  if ((value += 3) != 7 || (value -= 2) != 5) {
    return 4;
  }
  int expected = 5;
  if (!atomic_compare_exchange_strong(&value, &expected, 9)) {
    return 5;
  }
  expected = 8;
  if (atomic_compare_exchange_weak(&value, &expected, 10) || expected != 9) {
    return 11;
  }
  if (atomic_fetch_add(&value, 2) != 9 || atomic_load(&value) != 11) {
    return 6;
  }
  if (pointer++ != values || pointer != values + 1) {
    return 7;
  }
  if (atomic_fetch_add(&pointer, 2) != values + 1 ||
      pointer != values + 3) {
    return 10;
  }
  slots[0] = 3;
  if (slots[0]++ != 3 || slots[0] != 4) {
    return 12;
  }
  if (++holder.member != 2 || holder.member != 2) {
    return 13;
  }
  if (atomic_flag_test_and_set(&flag) || !atomic_flag_test_and_set(&flag)) {
    return 8;
  }
  atomic_flag_clear(&flag);
  return atomic_flag_test_and_set(&flag) ? 9 : 0;
}
