// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <cstddef>

struct Layout {
  char tag;
  int value;
};

constexpr std::size_t value_offset = offsetof(Layout, value);
static_assert(value_offset == 4);

int main() {
  return value_offset == 4 ? 0 : 1;
}
