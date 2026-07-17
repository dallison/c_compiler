// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <list>
#include <initializer_list>
#include <utility>

static bool is_odd(int x) { return (x % 2) != 0; }

int basics() {
  std::list<int> l;
  if (!l.empty() || l.size() != 0) return 1;
  l.push_back(2);
  l.push_back(3);
  l.push_front(1);
  if (l.size() != 3) return 2;
  if (l.front() != 1 || l.back() != 3) return 3;
  int expect = 1;
  for (std::list<int>::iterator it = l.begin(); it != l.end(); ++it) {
    if (*it != expect++) return 4;
  }
  // reverse iteration
  expect = 3;
  for (std::list<int>::reverse_iterator it = l.rbegin(); it != l.rend(); ++it) {
    if (*it != expect--) return 5;
  }
  l.pop_front();
  l.pop_back();
  if (l.size() != 1 || l.front() != 2) return 6;
  l.clear();
  if (!l.empty()) return 7;
  return 0;
}

int init_and_copy() {
  std::list<int> l = {5, 4, 3, 2, 1};
  if (l.size() != 5 || l.front() != 5 || l.back() != 1) return 10;

  std::list<int> copy = l;
  if (copy != l) return 11;
  copy.push_back(0);
  if (copy == l) return 12;
  if (copy.size() != 6) return 13;

  std::list<int> moved = std::move(copy);
  if (moved.size() != 6 || moved.back() != 0) return 14;
  if (!copy.empty()) return 15;

  std::list<int> assigned;
  assigned = moved;
  if (assigned != moved) return 16;

  std::list<int> count_list(4, 7);
  if (count_list.size() != 4) return 17;
  for (int v : count_list) {
    if (v != 7) return 18;
  }
  return 0;
}

int insert_erase() {
  std::list<int> l = {1, 2, 4, 5};
  std::list<int>::iterator it = l.begin();
  ++it;
  ++it;  // points at 4
  l.insert(it, 3);
  int expected[] = {1, 2, 3, 4, 5};
  int i = 0;
  for (int v : l) {
    if (v != expected[i++]) return 20;
  }
  if (i != 5) return 21;

  // erase the 3 we just inserted
  it = l.begin();
  ++it;
  ++it;  // points at 3
  it = l.erase(it);
  if (*it != 4) return 22;
  if (l.size() != 4) return 23;

  // range erase everything but the first
  std::list<int>::iterator second = l.begin();
  ++second;
  l.erase(second, l.end());
  if (l.size() != 1 || l.front() != 1) return 24;

  // insert range and count
  std::list<int> src = {8, 9};
  l.insert(l.end(), src.begin(), src.end());
  l.insert(l.end(), 2, 0);
  int expected2[] = {1, 8, 9, 0, 0};
  i = 0;
  for (int v : l) {
    if (v != expected2[i++]) return 25;
  }
  if (i != 5) return 26;
  return 0;
}

int emplace_resize() {
  std::list<std::pair<int, int>> l;
  l.emplace_back(1, 2);
  l.emplace_front(3, 4);
  l.emplace(l.end(), 5, 6);
  if (l.size() != 3) return 30;
  if (l.front().first != 3 || l.front().second != 4) return 31;
  if (l.back().first != 5 || l.back().second != 6) return 32;

  std::list<int> n = {1, 2, 3};
  n.resize(5);
  if (n.size() != 5 || n.back() != 0) return 33;
  n.resize(2);
  if (n.size() != 2 || n.back() != 2) return 34;
  n.resize(4, 9);
  if (n.size() != 4 || n.back() != 9) return 35;
  return 0;
}

int operations() {
  // remove / remove_if
  std::list<int> l = {1, 2, 2, 3, 2, 4};
  if (l.remove(2) != 3) return 40;
  int e1[] = {1, 3, 4};
  int i = 0;
  for (int v : l) {
    if (v != e1[i++]) return 41;
  }

  std::list<int> odds = {1, 2, 3, 4, 5, 6};
  if (odds.remove_if(is_odd) != 3) return 42;
  int e2[] = {2, 4, 6};
  i = 0;
  for (int v : odds) {
    if (v != e2[i++]) return 43;
  }

  // unique
  std::list<int> u = {1, 1, 2, 3, 3, 3, 2};
  if (u.unique() != 3) return 44;
  int e3[] = {1, 2, 3, 2};
  i = 0;
  for (int v : u) {
    if (v != e3[i++]) return 45;
  }

  // reverse
  std::list<int> r = {1, 2, 3, 4};
  r.reverse();
  int e4[] = {4, 3, 2, 1};
  i = 0;
  for (int v : r) {
    if (v != e4[i++]) return 46;
  }
  return 0;
}

int sort_merge_splice() {
  std::list<int> l = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
  l.sort();
  int i = 0;
  for (int v : l) {
    if (v != i++) return 50;
  }
  if (i != 10) return 51;

  // sort with custom comparator (descending)
  l.sort([](int a, int b) { return a > b; });
  i = 9;
  for (int v : l) {
    if (v != i--) return 52;
  }

  // merge two sorted lists
  std::list<int> a = {1, 3, 5};
  std::list<int> b = {2, 4, 6};
  a.merge(b);
  if (!b.empty()) return 53;
  int em[] = {1, 2, 3, 4, 5, 6};
  i = 0;
  for (int v : a) {
    if (v != em[i++]) return 54;
  }
  if (i != 6) return 55;

  // splice whole list
  std::list<int> x = {1, 2};
  std::list<int> y = {3, 4};
  std::list<int>::iterator pos = x.end();
  x.splice(pos, y);
  if (!y.empty() || x.size() != 4) return 56;
  int es[] = {1, 2, 3, 4};
  i = 0;
  for (int v : x) {
    if (v != es[i++]) return 57;
  }

  // splice single element
  std::list<int> p = {10, 20, 30};
  std::list<int> q = {99};
  x.splice(x.begin(), q, q.begin());
  if (!q.empty() || x.front() != 99) return 58;
  return 0;
}

int comparisons() {
  std::list<int> a = {1, 2, 3};
  std::list<int> b = {1, 2, 4};
  if (!(a < b)) return 60;
  if (b < a) return 61;
  if (!(a != b)) return 62;
  std::list<int> c = {1, 2, 3};
  if (!(a == c)) return 63;
  if (a > c || a < c) return 64;

  using std::swap;
  swap(a, b);
  if (a.back() != 4 || b.back() != 3) return 65;
  return 0;
}

int main() {
  int rc;
  if ((rc = basics()) != 0) return rc;
  if ((rc = init_and_copy()) != 0) return rc;
  if ((rc = insert_erase()) != 0) return rc;
  if ((rc = emplace_resize()) != 0) return rc;
  if ((rc = operations()) != 0) return rc;
  if ((rc = sort_merge_splice()) != 0) return rc;
  if ((rc = comparisons()) != 0) return rc;
  return 0;
}
