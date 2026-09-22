// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

int main() {
  std::multimap<int, int> mm;
  mm.insert(std::pair<const int, int>(1, 2));
  mm.insert(std::pair<const int, int>(1, 3));
  if (mm.size() != 2) {
    return 1;
  }

  std::multiset<int> ms;
  ms.insert(4);
  ms.insert(4);
  if (ms.size() != 2) {
    return 2;
  }

  std::unordered_multimap<int, int> umm;
  umm.insert(std::pair<const int, int>(5, 6));
  umm.insert(std::pair<const int, int>(5, 7));
  if (umm.size() != 2) {
    return 3;
  }

  std::unordered_multiset<int> ums;
  ums.insert(8);
  ums.insert(8);
  if (ums.size() != 2) {
    return 4;
  }
  return 0;
}
