// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Layout {
  char prefix;
  struct {
    int values[4];
  } impl;
};

constexpr unsigned long element_offset =
    __builtin_offsetof(Layout, impl.values[2]);
constexpr unsigned long expected_offset =
    __builtin_offsetof(Layout, impl.values) + 2 * sizeof(int);

static_assert(element_offset == expected_offset);

int main() {
  return 0;
}
