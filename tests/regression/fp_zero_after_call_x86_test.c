static int returns_true(void) {
  return 1;
}

static double zero_after_call(void) {
  if (returns_true()) {
    return 0.0;
  }
  return 1.5;
}

int main(void) {
  return zero_after_call() == 0.0 ? 0 : 1;
}
