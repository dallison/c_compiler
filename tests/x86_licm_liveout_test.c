__attribute__((noinline)) int licm_invariant(int value, int count) {
  int result = 0;
  for (int i = 0; i < count; ++i) {
    result += value * 9 + 7;
  }
  return result;
}

int main(void) {
  return licm_invariant(3, 4);
}
