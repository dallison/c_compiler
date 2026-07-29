// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <iterator>

int main() {
  int values[] = {2, 4, 6, 8};
  using Counted = std::counted_iterator<int*>;
  using Common =
      std::common_iterator<Counted, std::default_sentinel_t>;

  Common begin(Counted(values, 4));
  Common end(std::default_sentinel);
  if (begin == end || end != end || *begin != 2 ||
      begin.operator->() != values || end - begin != 4) {
    return 1;
  }

  Common old = begin++;
  if (*old != 2 || *begin != 4 || end - begin != 3) {
    return 2;
  }

  int sum = 0;
  for (Common current = begin; current != end; ++current) {
    sum += *current;
  }
  if (sum != 18 || std::ranges::distance(begin, end) != 3) {
    return 3;
  }

  using ConstCommon =
      std::common_iterator<std::counted_iterator<const int*>,
                           std::default_sentinel_t>;
  ConstCommon converted = begin;
  ConstCommon assigned(std::default_sentinel);
  assigned = begin;
  if (*converted != 4 || *assigned != 4) {
    return 4;
  }

  Common left(Counted(values, 1));
  Common right(Counted(values + 3, 1));
  iter_swap(left, right);
  if (values[0] != 8 || values[3] != 2) {
    return 5;
  }

  int&& moved = iter_move(left);
  if (moved != 8) {
    return 6;
  }

  return 0;
}
