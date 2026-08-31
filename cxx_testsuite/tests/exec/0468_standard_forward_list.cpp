// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <forward_list>
#include <initializer_list>
#include <utility>

static bool equals(const std::forward_list<int>& list,
                   const int* expected, int count) {
  std::forward_list<int>::const_iterator it = list.begin();
  for (int i = 0; i < count; ++i) {
    if (it == list.end() || *it != expected[i]) return false;
    ++it;
  }
  return it == list.end();
}

static bool is_odd(int value) { return (value & 1) != 0; }

static int basics() {
  std::forward_list<int> list;
  if (!list.empty() || list.begin() != list.end()) return 1;

  list.push_front(2);
  list.push_front(1);
  std::forward_list<int>::iterator pos = list.before_begin();
  pos = list.insert_after(pos, 0);
  if (*pos != 0 || list.front() != 0) return 2;
  int expected[] = {0, 1, 2};
  if (!equals(list, expected, 3)) return 3;

  list.pop_front();
  if (list.front() != 1) return 4;
  list.clear();
  if (!list.empty()) return 5;
  return 0;
}

static int construction_and_assignment() {
  std::forward_list<int> source = {1, 2, 3, 4};
  std::forward_list<int> copy(source);
  if (copy != source) return 10;

  std::forward_list<int> moved(std::move(copy));
  if (!copy.empty() || moved != source) return 11;

  std::forward_list<int> assigned;
  assigned = moved;
  if (assigned != moved) return 12;

  std::forward_list<int> move_assigned;
  move_assigned = std::move(assigned);
  if (!assigned.empty() || move_assigned != source) return 13;

  std::forward_list<int> repeated(3, 7);
  int sevens[] = {7, 7, 7};
  if (!equals(repeated, sevens, 3)) return 14;

  std::forward_list<int> defaults(2);
  int zeroes[] = {0, 0};
  if (!equals(defaults, zeroes, 2)) return 15;

  repeated.assign(source.begin(), source.end());
  if (repeated != source) return 16;
  repeated.assign(2, 9);
  int nines[] = {9, 9};
  if (!equals(repeated, nines, 2)) return 17;
  repeated = {5, 6};
  int last[] = {5, 6};
  if (!equals(repeated, last, 2)) return 18;
  return 0;
}

static int insert_erase_resize() {
  std::forward_list<int> list = {1, 5};
  std::forward_list<int>::iterator pos = list.begin();
  pos = list.insert_after(pos, 3, 2);
  if (*pos != 2) return 20;

  int middle[] = {3, 4};
  pos = list.insert_after(pos, middle, middle + 2);
  if (*pos != 4) return 21;
  list.insert_after(pos, {6, 7});
  int expected[] = {1, 2, 2, 2, 3, 4, 6, 7, 5};
  if (!equals(list, expected, 9)) return 22;

  std::forward_list<int>::iterator before = list.begin();
  list.erase_after(before);
  int erased[] = {1, 2, 2, 3, 4, 6, 7, 5};
  if (!equals(list, erased, 8)) return 23;

  std::forward_list<int>::iterator stop = list.begin();
  ++stop;
  ++stop;
  ++stop;
  list.erase_after(list.before_begin(), stop);
  int range_erased[] = {3, 4, 6, 7, 5};
  if (!equals(list, range_erased, 5)) return 24;

  list.resize(3);
  int shrunk[] = {3, 4, 6};
  if (!equals(list, shrunk, 3)) return 25;
  list.resize(5, 8);
  int grown[] = {3, 4, 6, 8, 8};
  if (!equals(list, grown, 5)) return 26;
  list.resize(0);
  if (!list.empty()) return 27;

  std::forward_list<std::pair<int, int>> pairs;
  pairs.emplace_front(1, 2);
  pairs.emplace_after(pairs.begin(), 3, 4);
  if (pairs.front().first != 1 || pairs.front().second != 2) return 28;
  std::forward_list<std::pair<int, int>>::iterator pair = pairs.begin();
  ++pair;
  if (pair->first != 3 || pair->second != 4) return 29;
  return 0;
}

static int operations() {
  std::forward_list<int> list = {1, 2, 2, 3, 2, 4};
  if (list.remove(2) != 3) return 30;
  int removed[] = {1, 3, 4};
  if (!equals(list, removed, 3)) return 31;

  std::forward_list<int> odds = {1, 2, 3, 4, 5, 6};
  if (odds.remove_if(is_odd) != 3) return 32;
  int evens[] = {2, 4, 6};
  if (!equals(odds, evens, 3)) return 33;

  std::forward_list<int> duplicates = {1, 1, 2, 3, 3, 3, 2};
  if (duplicates.unique() != 3) return 34;
  int unique[] = {1, 2, 3, 2};
  if (!equals(duplicates, unique, 4)) return 35;

  duplicates.reverse();
  int reversed[] = {2, 3, 2, 1};
  if (!equals(duplicates, reversed, 4)) return 36;

  std::forward_list<int> erased = {1, 2, 1, 3};
  if (std::erase(erased, 1) != 2) return 37;
  if (std::erase_if(erased, [](int value) { return value == 3; }) != 1)
    return 38;
  int left[] = {2};
  if (!equals(erased, left, 1)) return 39;
  return 0;
}

static int sort_merge_splice() {
  std::forward_list<int> list = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
  list.sort();
  int ascending[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  if (!equals(list, ascending, 10)) return 40;
  list.sort([](int left, int right) { return left > right; });
  int descending[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  if (!equals(list, descending, 10)) return 41;

  std::forward_list<int> a = {1, 3, 5};
  std::forward_list<int> b = {2, 4, 6};
  a.merge(b);
  int merged[] = {1, 2, 3, 4, 5, 6};
  if (!b.empty() || !equals(a, merged, 6)) return 42;

  std::forward_list<int> x = {1, 2};
  std::forward_list<int> y = {3, 4};
  x.splice_after(x.begin(), y);
  int whole[] = {1, 3, 4, 2};
  if (!y.empty() || !equals(x, whole, 4)) return 43;

  std::forward_list<int> source = {7, 8, 9};
  x.splice_after(x.before_begin(), source, source.before_begin());
  int single[] = {7, 1, 3, 4, 2};
  if (!equals(x, single, 5)) return 44;
  int source_left[] = {8, 9};
  if (!equals(source, source_left, 2)) return 45;

  x.splice_after(x.begin(), source, source.before_begin(), source.end());
  int range[] = {7, 8, 9, 1, 3, 4, 2};
  if (!source.empty() || !equals(x, range, 7)) return 46;
  return 0;
}

static int comparisons_and_swap() {
  std::forward_list<int> a = {1, 2, 3};
  std::forward_list<int> b = {1, 2, 4};
  std::forward_list<int> c = {1, 2, 3};
  if (!(a == c) || a != c) return 50;
  if (!(a < b) || b < a || !(b > a)) return 51;
  if (!(a <= c) || !(a >= c)) return 52;
  using std::swap;
  swap(a, b);
  int first[] = {1, 2, 4};
  int second[] = {1, 2, 3};
  if (!equals(a, first, 3) || !equals(b, second, 3)) return 53;
  return 0;
}

int main() {
  int rc;
  if ((rc = basics()) != 0) return rc;
  if ((rc = construction_and_assignment()) != 0) return rc;
  if ((rc = insert_erase_resize()) != 0) return rc;
  if ((rc = operations()) != 0) return rc;
  if ((rc = sort_merge_splice()) != 0) return rc;
  if ((rc = comparisons_and_swap()) != 0) return rc;
  return 0;
}
