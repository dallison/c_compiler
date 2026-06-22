// RUN: -std=c++20

constexpr int* pcode_return_heap_pointer(void) {
  int* value = new int(42);
  return value;
}

static_assert(*pcode_return_heap_pointer() == 42,
              "constexpr pcode heap pointer escape is invalid");
