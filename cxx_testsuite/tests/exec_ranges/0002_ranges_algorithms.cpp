// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <algorithm>
#include <ranges>
#include <utility>

struct record {
  int key;
  int payload;
};

int main() {
  int values[] = {5, 1, 4, 2, 3};
  std::ranges::sort(values);
  for (int i = 0; i < 5; ++i) {
    if (values[i] != i + 1) return 1;
  }
  if (!std::ranges::all_of(values, [](int value) { return value > 0; }))
    return 2;
  if (std::ranges::count_if(values,
                            [](int value) { return value % 2 == 0; }) != 2)
    return 3;
  if (std::ranges::find(values, 4) != values + 3) return 4;
  if (!std::ranges::binary_search(values, 3)) return 5;

  int doubled[5] = {};
  auto transformed = std::ranges::transform(
      values, doubled, [](int value) { return value * 2; });
  if (transformed.in != values + 5 || transformed.out != doubled + 5 ||
      doubled[4] != 10)
    return 6;

  int evens[5] = {};
  auto copied = std::ranges::copy_if(
      values, evens, [](int value) { return value % 2 == 0; });
  if (copied.out != evens + 2 || evens[0] != 2 || evens[1] != 4) return 7;
  std::ranges::reverse(values);
  if (values[0] != 5 || values[4] != 1) return 8;
  std::ranges::sort(values);
  record records[] = {{3, 30}, {1, 10}, {2, 20}};
  std::ranges::sort(records, std::ranges::less{}, &record::key);
  if (records[0].payload != 10 || records[2].payload != 30) return 9;

  auto minmax = std::ranges::minmax(values);
  if (minmax.min != 1 || minmax.max != 5) return 10;

  int left[] = {1, 3, 5};
  int right[] = {2, 4, 6};
  int merged[6] = {};
  auto merge_result = std::ranges::merge(left, right, merged);
  if (merge_result.out != merged + 6 || merged[0] != 1 || merged[5] != 6)
    return 11;

  int permutation[] = {1, 2, 3};
  auto permutation_result = std::ranges::next_permutation(permutation);
  if (!permutation_result.found || permutation[1] != 3) return 12;
  return 0;
}
