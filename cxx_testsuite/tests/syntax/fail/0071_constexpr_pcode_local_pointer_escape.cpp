// RUN: -std=c++20

constexpr int* pcode_return_local_pointer(void) {
  int value = 42;
  return &value;
}

constexpr int pcode_use_escaped_local_pointer(void) {
  int* value = pcode_return_local_pointer();
  return *value;
}

static_assert(pcode_use_escaped_local_pointer() == 42,
              "constexpr pcode local pointer escape is invalid");
