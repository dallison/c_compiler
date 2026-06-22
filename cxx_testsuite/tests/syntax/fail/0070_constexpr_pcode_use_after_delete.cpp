// RUN: -std=c++20

constexpr int pcode_use_after_delete(void) {
  int* value = new int(42);
  delete value;
  return *value;
}

static_assert(pcode_use_after_delete() == 42,
              "constexpr pcode use after delete is invalid");
