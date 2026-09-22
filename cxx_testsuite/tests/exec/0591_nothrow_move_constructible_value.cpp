// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

int main() {
  return std::is_nothrow_move_constructible<int>::value &&
                 std::is_nothrow_constructible<int, int&&>::value
             ? 0
             : 1;
}
