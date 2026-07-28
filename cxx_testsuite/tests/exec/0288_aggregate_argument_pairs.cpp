// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

struct Pair {
  const char* data;
  unsigned long size;
};

[[gnu::noinline]] bool register_pair_after_homed_argument(long first,
                                                          Pair pair) {
  const long* first_address = &first;
  return *first_address == 17 && pair.size == 3 && pair.data[2] == 't';
}

[[gnu::noinline]] bool stack_pair_in_leaf(long a0, long a1, long a2, long a3,
                                          long a4, long a5, long a6,
                                          Pair pair) {
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 == 28 &&
         pair.size == 3 && pair.data[0] == 'c';
}

int main() {
  Pair first{"cat", 3};
  if (!register_pair_after_homed_argument(17, first)) {
    return 1;
  }

  Pair second{"cat", 3};
  if (!stack_pair_in_leaf(1, 2, 3, 4, 5, 6, 7, second)) {
    return 2;
  }
  return 0;
}
