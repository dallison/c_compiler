// RUN: -std=c++20
// EXPECT_EXIT: 18

#include <ranges>
#include <vector>

int main() {
  std::vector<int> v{1, 2, 3, 4};

  // views::all on an lvalue container -> ref_view.
  auto rv = std::ranges::views::all(v);
  int s1 = 0;
  for (auto it = rv.begin(); it != rv.end(); ++it) s1 += *it;  // 10

  // all on a view (subrange) -> the view itself.
  std::ranges::subrange sr(v.begin(), v.end());
  auto rv2 = std::ranges::views::all(sr);
  int s2 = (int)rv2.size();  // 4

  // pipe: container | views::all -> ref_view.
  auto rv3 = v | std::ranges::views::all;
  int s3 = (int)rv3.size();  // 4

  return s1 + s2 + s3;  // 18
}
