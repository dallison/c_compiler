// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <tuple>

static_assert(!std::is_same_v<std::tuple<>, std::tuple<int>>);

int main() {
  std::tuple<> empty_left;
  std::tuple<> empty_right;
  if (!(empty_left == empty_right) || empty_left != empty_right ||
      empty_left < empty_right || empty_left > empty_right ||
      !(empty_left <= empty_right) || !(empty_left >= empty_right)) {
    return 7;
  }

  int a = 1;
  int b = 2;
  int c = 1;
  int d = 3;

  auto left = std::tie(a, b);
  auto right = std::tie(c, d);
  if (std::get<0>(left) != 1 || std::get<1>(left) != 2) {
    return 10;
  }
  if (std::get<0>(right) != 1 || std::get<1>(right) != 3) {
    return 11;
  }
  if (!(left < right)) {
    return 1;
  }
  if (right < left) {
    return 2;
  }
  if (!(left != right) || left == right) {
    return 3;
  }
  if (!(left <= right) || !(right > left) || !(right >= left)) {
    return 4;
  }
  d = 2;
  if (!(left == right)) {
    return 5;
  }

  auto value_left = std::make_tuple(1, 2);
  auto value_right = std::make_tuple(1L, 3L);
  return value_left < value_right && value_left != value_right ? 0 : 6;
}
