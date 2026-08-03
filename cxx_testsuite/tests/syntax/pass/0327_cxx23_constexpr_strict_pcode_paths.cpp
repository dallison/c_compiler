// RUN: -std=c++23 -fconstexpr-eval=pcode

constexpr int skip_goto(bool take) {
  if (take) {
    goto rejected;
  }
  return 7;
rejected:
  return 9;
}

constexpr int skip_invalid_operations(bool take) {
  if (take) {
    __builtin_trap();
  }
  try {
    if (take) {
      throw 1;
    }
  } catch (...) {
    return -1;
  }
  return 11;
}

struct cleanup_counter {
  int* count;
  constexpr ~cleanup_counter() { ++*count; }
};

constexpr int run_cleanup() {
  int count = 0;
  {
    cleanup_counter cleanup{&count};
  }
  return count;
}

static_assert(skip_goto(false) == 7);
static_assert(skip_invalid_operations(false) == 11);
static_assert(run_cleanup() == 1);
