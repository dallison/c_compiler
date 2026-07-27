// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <ranges>
#include <vector>

namespace adl_range {
struct sentinel {
  int* last;
};
struct range {
  int* first;
  int* last;
};
int* begin(range& value) { return value.first; }
sentinel end(range& value) { return sentinel{value.last}; }
bool operator!=(int* iterator, sentinel last) {
  return iterator != last.last;
}
}

int main() {
  int source[] = {1, 2, 3, 4};
  adl_range::range custom{source, source + 4};
  int adl_sum = 0;
  for (int value : custom) {
    adl_sum += value;
  }
  if (adl_sum != 10) return 1;

  int transformed = 0;
  for (int value :
       std::views::iota(0, 6) |
           std::views::transform([](int value) { return value * value; })) {
    transformed += value;
  }
  if (transformed != 55) return 2;

  std::vector<int> values{1, 2, 3, 4, 5, 6};
  int filtered = 0;
  for (int value : values |
                       std::views::filter([](int value) {
                         return value % 2 == 0;
                       }) |
                       std::views::take(2)) {
    filtered += value;
  }
  if (filtered != 6) return 3;

  int dropped = 0;
  for (int value : values | std::views::drop(3) |
                       std::views::take_while([](int value) {
                         return value < 6;
                       })) {
    dropped += value;
  }
  if (dropped != 9) return 4;

  int reversed = 0;
  for (int value : values | std::views::reverse | std::views::take(2)) {
    reversed = reversed * 10 + value;
  }
  if (reversed != 65) return 5;

  int array_sum = 0;
  for (int value : source | std::views::all) {
    array_sum += value;
  }
  if (array_sum != 10) return 6;

  auto counted = std::views::counted(source + 1, 2);
  if (std::ranges::distance(counted) != 2) return 7;
  return 0;
}
