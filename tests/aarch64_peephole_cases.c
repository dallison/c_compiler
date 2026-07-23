__attribute__((noinline)) int move_chain(int value) {
  int first = value;
  int second = first;
  return second;
}

__attribute__((noinline)) int branch_zero(int value) {
  if (value & 4) {
    return 7;
  }
  return 3;
}

__attribute__((noinline)) unsigned long zero_extend_byte(unsigned char value) {
  return value;
}

__attribute__((noinline)) long shifted_add(long value) {
  return value * 3;
}

__attribute__((noinline)) int load_small_offset(const int* pointer) {
  return pointer[3];
}

__attribute__((noinline)) int load_large_offset(const int* pointer) {
  return pointer[100];
}
