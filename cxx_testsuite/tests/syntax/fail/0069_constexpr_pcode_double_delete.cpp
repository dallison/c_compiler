// RUN: -std=c++20

constexpr int pcode_double_delete(void) {
  int* value = new int(42);
  delete value;
  delete value;
  return 42;
}

static_assert(pcode_double_delete() == 42,
              "constexpr pcode double delete is invalid");
