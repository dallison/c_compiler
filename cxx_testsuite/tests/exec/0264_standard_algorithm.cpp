// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <utility>

static bool is_even(int x) { return x % 2 == 0; }

int min_max_clamp() {
  if (std::min(3, 7) != 3 || std::max(3, 7) != 7) return 1;
  if (std::min(7, 3) != 3 || std::max(7, 3) != 7) return 2;
  if (std::min({4, 1, 3, 2}) != 1 || std::max({4, 1, 3, 2}) != 4) return 3;
  std::pair<int, int> mm = std::minmax({5, 2, 9, 1, 7});
  if (mm.first != 1 || mm.second != 9) return 4;
  if (std::clamp(5, 1, 10) != 5) return 5;
  if (std::clamp(-3, 1, 10) != 1) return 6;
  if (std::clamp(42, 1, 10) != 10) return 7;
  return 0;
}

int sorting() {
  int a[] = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
  std::sort(a, a + 10);
  for (int i = 0; i < 10; ++i) {
    if (a[i] != i) return 10;
  }
  if (!std::is_sorted(a, a + 10)) return 11;
  std::sort(a, a + 10, [](int x, int y) { return x > y; });
  for (int i = 0; i < 10; ++i) {
    if (a[i] != 9 - i) return 12;
  }
  // stable_sort keeps equal elements in order; sort by value/2 keys.
  std::pair<int, int> b[] = {{1, 0}, {0, 1}, {1, 2}, {0, 3}, {1, 4}};
  std::stable_sort(b, b + 5,
                   [](const std::pair<int, int>& x,
                      const std::pair<int, int>& y) { return x.first < y.first; });
  if (b[0].second != 1 || b[1].second != 3) return 13;
  if (b[2].second != 0 || b[3].second != 2 || b[4].second != 4) return 14;

  int c[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  std::partial_sort(c, c + 3, c + 10);
  if (c[0] != 0 || c[1] != 1 || c[2] != 2) return 15;

  int d[] = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
  std::nth_element(d, d + 5, d + 10);
  if (d[5] != 5) return 16;
  for (int i = 0; i < 5; ++i) {
    if (d[i] > 5) return 17;
  }
  return 0;
}

int searching() {
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8};
  if (std::find(a, a + 8, 5) != a + 4) return 20;
  if (std::find(a, a + 8, 99) != a + 8) return 21;
  if (std::find_if(a, a + 8, is_even) != a + 1) return 22;
  if (std::find_if_not(a, a + 8, is_even) != a) return 23;
  if (std::count_if(a, a + 8, is_even) != 4) return 24;
  if (std::count(a, a + 8, 3) != 1) return 25;
  if (!std::all_of(a, a + 8, [](int x) { return x > 0; })) return 26;
  if (std::any_of(a, a + 8, [](int x) { return x > 100; })) return 27;
  if (!std::none_of(a, a + 8, [](int x) { return x < 0; })) return 28;

  // binary search family (a is sorted)
  if (std::lower_bound(a, a + 8, 5) != a + 4) return 29;
  if (std::upper_bound(a, a + 8, 5) != a + 5) return 30;
  if (!std::binary_search(a, a + 8, 6)) return 31;
  if (std::binary_search(a, a + 8, 0)) return 32;
  std::pair<int*, int*> r = std::equal_range(a, a + 8, 4);
  if (r.first != a + 3 || r.second != a + 4) return 33;

  if (*std::min_element(a, a + 8) != 1) return 34;
  if (*std::max_element(a, a + 8) != 8) return 35;
  std::pair<int*, int*> me = std::minmax_element(a, a + 8);
  if (*me.first != 1 || *me.second != 8) return 36;

  int hay[] = {1, 2, 3, 2, 3, 4};
  int need[] = {2, 3};
  if (std::search(hay, hay + 6, need, need + 2) != hay + 1) return 37;
  if (std::find_end(hay, hay + 6, need, need + 2) != hay + 3) return 38;
  int any_of_these[] = {7, 3};
  if (std::find_first_of(hay, hay + 6, any_of_these, any_of_these + 2) != hay + 2)
    return 39;
  int adj[] = {1, 2, 2, 3};
  if (std::adjacent_find(adj, adj + 4) != adj + 1) return 40;
  int runs[] = {1, 1, 1, 2};
  if (std::search_n(runs, runs + 4, 3, 1) != runs) return 41;
  return 0;
}

int modifying() {
  int src[] = {1, 2, 3, 4, 5};
  int dst[5] = {0, 0, 0, 0, 0};
  std::copy(src, src + 5, dst);
  for (int i = 0; i < 5; ++i) {
    if (dst[i] != src[i]) return 50;
  }
  int evens[5] = {0, 0, 0, 0, 0};
  int* end = std::copy_if(src, src + 5, evens, is_even);
  if (end - evens != 2 || evens[0] != 2 || evens[1] != 4) return 51;

  int back[5] = {0, 0, 0, 0, 0};
  std::copy_backward(src, src + 5, back + 5);
  if (back[4] != 5 || back[0] != 1) return 52;

  int fillbuf[4];
  std::fill(fillbuf, fillbuf + 4, 9);
  if (fillbuf[0] != 9 || fillbuf[3] != 9) return 53;

  int t[5];
  std::transform(src, src + 5, t, [](int x) { return x * x; });
  if (t[0] != 1 || t[4] != 25) return 54;
  int t2[5];
  std::transform(src, src + 5, src, t2, [](int x, int y) { return x + y; });
  if (t2[2] != 6) return 55;

  int rm[] = {1, 2, 2, 3, 2, 4};
  int* rend = std::remove(rm, rm + 6, 2);
  if (rend - rm != 3 || rm[0] != 1 || rm[1] != 3 || rm[2] != 4) return 56;

  int uq[] = {1, 1, 2, 2, 2, 3, 1};
  int* uend = std::unique(uq, uq + 7);
  if (uend - uq != 4 || uq[0] != 1 || uq[1] != 2 || uq[2] != 3 || uq[3] != 1)
    return 57;

  int rev[] = {1, 2, 3, 4};
  std::reverse(rev, rev + 4);
  if (rev[0] != 4 || rev[3] != 1) return 58;

  int rot[] = {1, 2, 3, 4, 5};
  std::rotate(rot, rot + 2, rot + 5);
  if (rot[0] != 3 || rot[2] != 5 || rot[3] != 1 || rot[4] != 2) return 59;

  int rep[] = {1, 2, 1, 3, 1};
  std::replace(rep, rep + 5, 1, 9);
  if (rep[0] != 9 || rep[2] != 9 || rep[4] != 9 || rep[1] != 2) return 60;

  int sw1[] = {1, 2, 3};
  int sw2[] = {4, 5, 6};
  std::swap_ranges(sw1, sw1 + 3, sw2);
  if (sw1[0] != 4 || sw2[0] != 1) return 61;
  return 0;
}

int partitioning() {
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8};
  int* p = std::partition(a, a + 8, is_even);
  for (int* it = a; it != p; ++it) {
    if (!is_even(*it)) return 70;
  }
  for (int* it = p; it != a + 8; ++it) {
    if (is_even(*it)) return 71;
  }

  int b[] = {1, 2, 3, 4, 5, 6};
  int* sp = std::stable_partition(b, b + 6, is_even);
  if (sp - b != 3) return 72;
  if (b[0] != 2 || b[1] != 4 || b[2] != 6) return 73;
  if (b[3] != 1 || b[4] != 3 || b[5] != 5) return 74;

  int c[] = {2, 4, 6, 1, 3, 5};
  if (!std::is_partitioned(c, c + 6, is_even)) return 75;
  int* pp = std::partition_point(c, c + 6, is_even);
  if (pp - c != 3) return 76;
  return 0;
}

int heaps() {
  int a[] = {3, 1, 4, 1, 5, 9, 2, 6};
  std::make_heap(a, a + 8);
  if (!std::is_heap(a, a + 8)) return 80;
  if (a[0] != 9) return 81;  // max at root
  std::pop_heap(a, a + 8);
  if (a[7] != 9) return 82;
  // push a new large value
  a[7] = 10;
  std::push_heap(a, a + 8);
  if (a[0] != 10) return 83;
  std::sort_heap(a, a + 8);
  if (!std::is_sorted(a, a + 8)) return 84;
  return 0;
}

int set_ops() {
  int x[] = {1, 2, 3, 4, 5};
  int y[] = {3, 4, 5, 6, 7};
  int out[10];
  int* e;

  e = std::set_union(x, x + 5, y, y + 5, out);
  if (e - out != 7 || out[0] != 1 || out[6] != 7) return 90;

  e = std::set_intersection(x, x + 5, y, y + 5, out);
  if (e - out != 3 || out[0] != 3 || out[2] != 5) return 91;

  e = std::set_difference(x, x + 5, y, y + 5, out);
  if (e - out != 2 || out[0] != 1 || out[1] != 2) return 92;

  e = std::set_symmetric_difference(x, x + 5, y, y + 5, out);
  if (e - out != 4 || out[0] != 1 || out[3] != 7) return 93;

  int m1[] = {1, 3, 5};
  int m2[] = {2, 4, 6};
  e = std::merge(m1, m1 + 3, m2, m2 + 3, out);
  if (e - out != 6) return 94;
  if (!std::is_sorted(out, out + 6)) return 95;

  int sub[] = {4, 5};
  if (!std::includes(y, y + 5, sub, sub + 2)) return 96;
  int notsub[] = {9};
  if (std::includes(y, y + 5, notsub, notsub + 1)) return 97;
  return 0;
}

int compare_and_perm() {
  int a[] = {1, 2, 3};
  int b[] = {1, 2, 4};
  if (!std::lexicographical_compare(a, a + 3, b, b + 3)) return 100;
  if (std::lexicographical_compare(b, b + 3, a, a + 3)) return 101;
  if (!std::equal(a, a + 3, a)) return 102;
  if (std::equal(a, a + 3, b)) return 103;
  std::pair<int*, int*> mm = std::mismatch(a, a + 3, b);
  if (mm.first != a + 2) return 104;

  int p[] = {1, 2, 3};
  bool r = std::next_permutation(p, p + 3);
  if (!r || p[0] != 1 || p[1] != 3 || p[2] != 2) return 105;
  std::prev_permutation(p, p + 3);
  if (p[0] != 1 || p[1] != 2 || p[2] != 3) return 106;

  int q1[] = {1, 2, 3, 4};
  int q2[] = {4, 3, 2, 1};
  if (!std::is_permutation(q1, q1 + 4, q2)) return 107;
  int q3[] = {1, 2, 3, 5};
  if (std::is_permutation(q1, q1 + 4, q3)) return 108;
  return 0;
}

int main(void) {
  int rc;
  if ((rc = min_max_clamp()) != 0) return rc;
  if ((rc = sorting()) != 0) return rc;
  if ((rc = searching()) != 0) return rc;
  if ((rc = modifying()) != 0) return rc;
  if ((rc = partitioning()) != 0) return rc;
  if ((rc = heaps()) != 0) return rc;
  if ((rc = set_ops()) != 0) return rc;
  if ((rc = compare_and_perm()) != 0) return rc;
  return 0;
}
