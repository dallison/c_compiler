static_assert(sizeof(bool) == 1);
static_assert(alignof(bool) == 1);
struct BoolAbiLayout {
  bool value;
  char next;
};
static_assert(sizeof(BoolAbiLayout) == 2);

struct Accumulator {
  int value;

  explicit Accumulator(int initial) : value(initial) {}

  void add(int amount) {
    value += amount;
  }
};

[[gnu::noinline]] int cpp_copy(int value) {
  Accumulator first(value);
  Accumulator second = first;
  return second.value;
}

[[gnu::noinline]] int cpp_loop(int value, int count) {
  Accumulator result(0);
  for (int i = 0; i < count; ++i) {
    result.add(value * 5 + i);
  }
  return result.value;
}

int main() {
  int result = 0;
  if (cpp_copy(37) != 37) {
    result |= 1;
  }
  if (cpp_loop(3, 4) != 66 || cpp_loop(3, 0) != 0) {
    result |= 2;
  }
  return result;
}
