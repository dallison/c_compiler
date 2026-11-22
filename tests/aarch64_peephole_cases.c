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
  return pointer[5000];
}

__attribute__((noinline)) void store_zero32(unsigned int* pointer) {
  *pointer = 0;
}

__attribute__((noinline)) void store_zero64(unsigned long* pointer) {
  *pointer = 0;
}

__attribute__((noinline)) int call_forward_callee(int a, int b);

__attribute__((noinline)) int call_forward(int a, int b) {
  return call_forward_callee(a, b);
}

__attribute__((noinline)) int call_swap(int a, int b) {
  return call_forward_callee(b, a);
}

__attribute__((noinline)) int short_circuit_chain(int a, int b, int c, int d) {
  if ((a && (b & 4)) || (c && (d & 8))) {
    return 19;
  }
  return 7;
}

__attribute__((noinline)) int odd_callee_saved_frame(int a, int b, int c) {
  int first = call_forward_callee(a, b);
  int second = call_forward_callee(b, c);
  return first + second + c;
}
