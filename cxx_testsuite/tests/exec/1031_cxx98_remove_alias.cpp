// RUN: -std=c++98
// EXPECT_EXIT: 0

#include <algorithm>

int main() {
  int values[] = {2, 1, 2, 3, 2};
  int* removed = std::remove(values, values + 5, values[0]);
  if (removed - values != 2 || values[0] != 1 || values[1] != 3) return 1;
  return 0;
}
