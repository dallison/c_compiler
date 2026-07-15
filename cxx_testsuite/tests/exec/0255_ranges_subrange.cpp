// RUN: -std=c++20
// EXPECT_EXIT: 106

#include <ranges>

using namespace std;

int main() {
  int a[5] = {10, 20, 30, 40, 50};

  // CTAD from iterator + sentinel.
  ranges::subrange sr(a + 0, a + 5);

  int total = 0;
  total += (int)sr.size();       // 5   (sized_sentinel_for)
  total += sr.empty() ? 0 : 1;   // +1  (non-empty)
  total += (int)sr.front();      // +10 (view_interface::front)
  total += (int)sr.back();       // +50 (view_interface::back, bidi+common)
  total += (int)sr[2];           // +30 (view_interface::operator[])

  // view_interface::data + contiguous_iterator constraint.
  int* p = sr.data();
  total += (int)(*p);            // +10

  return total;                  // 5 + 1 + 10 + 50 + 30 + 10 = 106
}
