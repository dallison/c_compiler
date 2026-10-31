// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <algorithm>

int main() {
  int values[] = {2, 1, 2, 3, 2};
  int* removed = std::remove(values, values + 5, values[0]);
  if (removed - values != 2 || values[0] != 1 || values[1] != 3) return 1;

  int replaced[] = {1, 2, 1, 3};
  std::replace(replaced, replaced + 4, replaced[0], replaced[1]);
  if (replaced[0] != 2 || replaced[1] != 2 || replaced[2] != 2 ||
      replaced[3] != 3) {
    return 2;
  }

  int conditional[] = {1, 9, 1};
  std::replace_if(
      conditional, conditional + 3,
      [](int value) { return value == 1; }, conditional[1]);
  if (conditional[0] != 9 || conditional[1] != 9 || conditional[2] != 9) {
    return 3;
  }
  return 0;
}
