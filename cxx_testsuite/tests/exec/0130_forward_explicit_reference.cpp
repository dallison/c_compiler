// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <utility>

int main(void) {
  int value = 7;
  int& ref = std::forward<int&>(value);
  ref = 11;
  if (value != 11) {
    return 1;
  }

  int&& moved = std::forward<int>(value);
  if (moved != 11) {
    return 2;
  }
  return 0;
}
