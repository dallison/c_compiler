// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <ranges>
#include <tuple>

int main() {
  int left[] = {1, 2, 3, 4};
  int right[] = {10, 20, 30};

  auto zipped = std::views::zip(left, right);
  if (zipped.size() != 3) {
    return 1;
  }

  int count = 0;
  int total = 0;
  for (auto values : zipped) {
    total += std::get<0>(values) + std::get<1>(values);
    std::get<0>(values) += 1;
    ++count;
  }
  if (count != 3 || total != 66 ||
      left[0] != 2 || left[1] != 3 || left[2] != 4 || left[3] != 4) {
    return 2;
  }

  int third[] = {100, 200, 300};
  int fourth[] = {1000, 2000, 3000};
  auto four_way = std::views::zip(left, right, third, fourth);
  auto first = *four_way.begin();
  if (std::get<0>(first) != 2 || std::get<1>(first) != 10 ||
      std::get<2>(first) != 100 || std::get<3>(first) != 1000) {
    return 3;
  }

  auto transformed = std::views::zip_transform(
      [](int a, int b) { return a * b; }, left, right);
  total = 0;
  for (int value : transformed) {
    total += value;
  }
  if (total != 200) {
    return 4;
  }

  count = 0;
  total = 0;
  for (auto [index, value] : std::views::enumerate(right)) {
    if (index != count) {
      return 5;
    }
    total += value;
    ++count;
  }
  if (count != 3 || total != 60) {
    return 6;
  }

  total = 0;
  for (auto entry : right | std::views::enumerate) {
    total += static_cast<int>(std::get<0>(entry)) * std::get<1>(entry);
  }
  if (total != 80) {
    return 7;
  }

  return 0;
}
