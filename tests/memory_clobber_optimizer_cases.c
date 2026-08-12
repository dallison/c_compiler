static int* barrier_pointer;
static int barrier_value = 4;

__attribute__((noinline)) int memory_clobber_loop(int count) {
  int result = 0;
  for (int i = 0; i < count; ++i) {
    asm volatile("" ::: "memory");
    result += barrier_value;
  }
  return result;
}

__attribute__((noinline)) int memory_clobber_reload(void) {
  int value = 5;
  barrier_pointer = &value;
  asm volatile("" ::: "memory");
  return value;
}
