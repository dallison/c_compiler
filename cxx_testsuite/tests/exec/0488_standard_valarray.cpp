// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <valarray>

int main() {
  int initial[] = {1, 2, 3, 4, 5, 6};
  std::valarray<int> values(initial, 6);
  std::valarray<int> result(values);
  result *= 2;
  result += 1;
  if (result.size() != 6) return 11;
  if (result.sum() != 48) return 12;
  if (result.min() != 3) return 13;
  if (result.max() != 13) return 14;

  values[std::slice(1, 3, 2)] = 9;
  if (values[1] != 9 || values[3] != 9 || values[5] != 9) return 2;

  std::valarray<bool> mask = values > 5;
  std::valarray<int> selected =
      static_cast<const std::valarray<int>&>(values)[mask];
  if (selected.size() != 3 || selected.sum() != 27) return 3;

  std::valarray<std::size_t> indices = {5, 0, 3};
  std::valarray<int> indirect =
      static_cast<const std::valarray<int>&>(values)[indices];
  if (indirect[0] != 9 || indirect[1] != 1 || indirect[2] != 9) return 4;

  std::valarray<int> shifted = values.shift(2);
  std::valarray<int> rotated = values.cshift(-1);
  if (shifted[0] != values[2] || shifted[5] != 0 ||
      rotated[0] != values[5]) return 5;
  return 0;
}
