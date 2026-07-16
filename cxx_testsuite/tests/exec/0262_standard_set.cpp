// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <set>
#include <initializer_list>
#include <utility>

int basic_insert_lookup() {
  std::set<int> s;
  if (!s.empty() || s.size() != 0) {
    return 1;
  }
  std::pair<std::set<int>::iterator, bool> a = s.insert(5);
  if (!a.second || *a.first != 5) {
    return 2;
  }
  s.insert(2);
  s.insert(8);
  std::pair<std::set<int>::iterator, bool> dup = s.insert(5);
  if (dup.second || *dup.first != 5) {
    return 3;
  }
  if (s.size() != 3 || s.empty()) {
    return 4;
  }
  if (!s.contains(2) || !s.contains(5) || !s.contains(8) || s.contains(9)) {
    return 5;
  }
  if (s.count(5) != 1 || s.count(9) != 0) {
    return 6;
  }
  if (s.find(8) == s.end() || *s.find(8) != 8 || s.find(9) != s.end()) {
    return 7;
  }
  return 0;
}

int ordered_iteration() {
  std::set<int> s = {5, 1, 4, 2, 3, 1, 4};
  if (s.size() != 5) {
    return 10;
  }
  int expected = 1;
  for (std::set<int>::iterator it = s.begin(); it != s.end(); ++it) {
    if (*it != expected) {
      return 11;
    }
    expected++;
  }
  if (expected != 6) {
    return 12;
  }
  // reverse iteration
  int r = 5;
  for (std::set<int>::reverse_iterator it = s.rbegin(); it != s.rend(); ++it) {
    if (*it != r) {
      return 13;
    }
    r--;
  }
  if (r != 0) {
    return 14;
  }
  return 0;
}

int erase_variants() {
  std::set<int> s = {1, 2, 3, 4, 5, 6};
  if (s.erase(3) != 1 || s.erase(100) != 0) {
    return 20;
  }
  if (s.size() != 5 || s.contains(3)) {
    return 21;
  }
  std::set<int>::iterator it = s.find(4);
  std::set<int>::iterator next = s.erase(it);
  if (*next != 5 || s.contains(4)) {
    return 22;
  }
  // erase a range [2, 6)
  std::set<int>::iterator first = s.find(2);
  std::set<int>::iterator last = s.find(6);
  s.erase(first, last);
  if (s.size() != 2 || !s.contains(1) || !s.contains(6) || s.contains(2) ||
      s.contains(5)) {
    return 23;
  }
  s.clear();
  if (!s.empty() || s.size() != 0) {
    return 24;
  }
  return 0;
}

int bounds_and_ranges() {
  std::set<int> s = {10, 20, 30, 40, 50};
  if (*s.lower_bound(20) != 20 || *s.lower_bound(25) != 30) {
    return 30;
  }
  if (*s.upper_bound(20) != 30 || *s.upper_bound(25) != 30) {
    return 31;
  }
  if (s.lower_bound(50) == s.end() || s.upper_bound(50) != s.end()) {
    return 32;
  }
  std::pair<std::set<int>::iterator, std::set<int>::iterator> r =
      s.equal_range(30);
  if (*r.first != 30 || *r.second != 40) {
    return 33;
  }
  std::pair<std::set<int>::iterator, std::set<int>::iterator> empty =
      s.equal_range(35);
  if (empty.first != empty.second || *empty.first != 40) {
    return 34;
  }
  return 0;
}

int copy_move_semantics() {
  std::set<int> a = {1, 2, 3};
  std::set<int> b = a;  // copy
  if (b.size() != 3 || !b.contains(2)) {
    return 40;
  }
  b.insert(4);
  if (a.size() != 3 || a.contains(4)) {
    return 41;  // copy must be independent
  }
  std::set<int> c = std::move(b);  // move
  if (c.size() != 4 || !c.contains(4)) {
    return 42;
  }
  std::set<int> d;
  d = a;  // copy assign
  if (d.size() != 3) {
    return 43;
  }
  std::set<int> e;
  e = std::move(c);  // move assign
  if (e.size() != 4 || !e.contains(4)) {
    return 44;
  }
  a.swap(e);
  if (a.size() != 4 || e.size() != 3) {
    return 45;
  }
  std::swap(a, e);
  if (a.size() != 3 || e.size() != 4) {
    return 46;
  }
  return 0;
}

int custom_comparator() {
  std::set<int, std::greater<int> > s = {1, 2, 3, 4};
  // greater ordering => descending
  int expected = 4;
  for (std::set<int, std::greater<int> >::iterator it = s.begin();
       it != s.end(); ++it) {
    if (*it != expected) {
      return 50;
    }
    expected--;
  }
  if (expected != 0) {
    return 51;
  }
  if (*s.begin() != 4 || *s.lower_bound(3) != 3) {
    return 52;
  }
  return 0;
}

int transparent_lookup() {
  std::set<int, std::less<> > s = {10, 20, 30};
  // heterogeneous lookup with a long key against a set<int>
  long key = 20;
  if (s.find(key) == s.end() || *s.find(key) != 20) {
    return 60;
  }
  if (!s.contains(key) || s.count(key) != 1) {
    return 61;
  }
  return 0;
}

int emplace_and_nodes() {
  std::set<int> s;
  std::pair<std::set<int>::iterator, bool> e = s.emplace(7);
  if (!e.second || *e.first != 7) {
    return 70;
  }
  if (s.emplace(7).second) {
    return 71;  // duplicate emplace fails
  }
  s.insert(3);
  s.insert(9);
  // extract a node and re-insert into another set
  std::set<int>::node_type nh = s.extract(9);
  if (nh.empty() || nh.value() != 9 || s.contains(9) || s.size() != 2) {
    return 72;
  }
  std::set<int> other;
  std::set<int>::insert_return_type ret = other.insert(std::move(nh));
  if (!ret.inserted || *ret.position != 9 || !other.contains(9)) {
    return 73;
  }
  return 0;
}

int merge_sets() {
  std::set<int> a = {1, 2, 3};
  std::set<int> b = {3, 4, 5};
  a.merge(b);
  if (a.size() != 5 || !a.contains(4) || !a.contains(5)) {
    return 80;
  }
  // 3 was a duplicate, so it stays in b
  if (b.size() != 1 || !b.contains(3)) {
    return 81;
  }
  return 0;
}

int comparisons_and_erase_if() {
  std::set<int> a = {1, 2, 3};
  std::set<int> b = {1, 2, 3};
  std::set<int> c = {1, 2, 4};
  if (!(a == b) || a != b) {
    return 90;
  }
  if (a == c) {
    return 91;
  }
  if (!(a < c) || !(c > a)) {
    return 92;
  }
  std::set<int> d = {1, 2, 3, 4, 5, 6};
  std::size_t removed = std::erase_if(d, [](int x) { return x % 2 == 0; });
  if (removed != 3 || d.size() != 3 || d.contains(2) || !d.contains(1)) {
    return 93;
  }
  return 0;
}

struct Point {
  int x;
  int y;
  Point() : x(0), y(0) {}
  Point(int a, int b) : x(a), y(b) {}
  bool operator<(const Point& other) const {
    if (x != other.x) {
      return x < other.x;
    }
    return y < other.y;
  }
};

int set_of_struct() {
  std::set<Point> s;
  s.insert(Point(1, 2));
  s.insert(Point(1, 1));
  s.insert(Point(0, 5));
  s.insert(Point(1, 2));  // duplicate
  if (s.size() != 3) {
    return 100;
  }
  std::set<Point>::iterator it = s.begin();
  if (it->x != 0 || it->y != 5) {
    return 101;
  }
  ++it;
  if (it->x != 1 || it->y != 1) {
    return 102;
  }
  if (!s.contains(Point(1, 2)) || s.contains(Point(9, 9))) {
    return 103;
  }
  return 0;
}

int main(void) {
  int rc;
  if ((rc = basic_insert_lookup()) != 0) return rc;
  if ((rc = ordered_iteration()) != 0) return rc;
  if ((rc = erase_variants()) != 0) return rc;
  if ((rc = bounds_and_ranges()) != 0) return rc;
  if ((rc = copy_move_semantics()) != 0) return rc;
  if ((rc = custom_comparator()) != 0) return rc;
  if ((rc = transparent_lookup()) != 0) return rc;
  if ((rc = emplace_and_nodes()) != 0) return rc;
  if ((rc = merge_sets()) != 0) return rc;
  if ((rc = comparisons_and_erase_if()) != 0) return rc;
  if ((rc = set_of_struct()) != 0) return rc;
  return 0;
}
