// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

struct Pair {
  int* first;
  int* second;
};

int main() {
  using layout_type = typename std::is_standard_layout<Pair>::type;
  return layout_type::value ? 0 : 1;
}
