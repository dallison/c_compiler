// RUN: -std=c++20
#define __ATOMIC_SEQ_CST 5

int atomic_intrinsic_values(void) {
  int value = 3;
  int loaded = __atomic_load_n(&value, __ATOMIC_SEQ_CST);
  __atomic_store_n(&value, loaded + 1, __ATOMIC_SEQ_CST);
  int old_add = __atomic_fetch_add(&value, 2, __ATOMIC_SEQ_CST);
  int new_sub = __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST);
  int sync_old = __sync_fetch_and_add(&value, 1);
  int sync_new = __sync_sub_and_fetch(&value, 1);
  int expected = sync_new;
  bool exchanged = __atomic_compare_exchange_n(
      &value, &expected, 12, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  int old_cas = __sync_val_compare_and_swap(&value, 12, 13);
  bool bool_cas = __sync_bool_compare_and_swap(&value, 13, 14);
  __sync_synchronize();
  return loaded + old_add + new_sub + sync_old + sync_new + exchanged +
         old_cas + bool_cas + value;
}
