int main(void) {
  int value = 4;
  if (__atomic_load_n(&value, __ATOMIC_SEQ_CST) != 4) {
    return 1;
  }
  __atomic_store_n(&value, 7, __ATOMIC_SEQ_CST);
  if (value != 7) {
    return 2;
  }
  if (__atomic_fetch_add(&value, 3, __ATOMIC_SEQ_CST) != 7) {
    return 3;
  }
  if (value != 10) {
    return 4;
  }
  if (__atomic_add_fetch(&value, 2, __ATOMIC_SEQ_CST) != 12) {
    return 5;
  }
  if (__sync_fetch_and_sub(&value, 5) != 12) {
    return 6;
  }
  if (value != 7) {
    return 7;
  }
  if (__sync_sub_and_fetch(&value, 2) != 5) {
    return 8;
  }
  if (!__sync_bool_compare_and_swap(&value, 5, 9)) {
    return 9;
  }
  if (value != 9) {
    return 10;
  }
  if (__sync_val_compare_and_swap(&value, 4, 11) != 9) {
    return 11;
  }
  if (value != 9) {
    return 12;
  }
  int expected = 4;
  if (__atomic_compare_exchange_n(&value, &expected, 13, false,
                                  __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
    return 13;
  }
  if (expected != 9 || value != 9) {
    return 14;
  }
  if (!__atomic_compare_exchange_n(&value, &expected, 15, false,
                                   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
    return 15;
  }
  if (value != 15) {
    return 16;
  }

  unsigned char byte = 250;
  if (__atomic_fetch_add(&byte, 10, __ATOMIC_SEQ_CST) != 250 || byte != 4) {
    return 17;
  }
  unsigned char expected_byte = 4;
  if (!__atomic_compare_exchange_n(&byte, &expected_byte, 9, false,
                                   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ||
      byte != 9) {
    return 18;
  }

  unsigned short half = 65530;
  if (__atomic_add_fetch(&half, 10, __ATOMIC_SEQ_CST) != 4 || half != 4) {
    return 19;
  }
  unsigned short expected_half = 3;
  if (__atomic_compare_exchange_n(&half, &expected_half, 12, false,
                                  __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ||
      expected_half != 4 || half != 4) {
    return 20;
  }
  return 0;
}
