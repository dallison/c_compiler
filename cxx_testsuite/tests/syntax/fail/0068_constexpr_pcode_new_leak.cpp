// RUN: -std=c++20

constexpr int pcode_new_leak(void) {
  int* value = new int(42);
  return *value;
}

static_assert(pcode_new_leak() == 42,
              "constexpr pcode new allocations must be deleted");
