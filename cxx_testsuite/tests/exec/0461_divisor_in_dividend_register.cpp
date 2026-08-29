// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// An x86-64 divide takes its dividend in rax and rdx, and the emitter writes
// both before the divide runs.  A divisor that the allocator had put in either
// was therefore overwritten before the divide read it, and `idivq %rax`
// divided the dividend by itself.  Building a heap is enough to reach it: the
// element count is a pointer difference divided by the element size, and one
// of those divisions came out as 1.

struct Less {
  bool operator()(const int& a, const int& b) const { return a < b; }
};

template <class It, class Diff, class Compare>
static void sift_down(It first, Diff start, Diff n, Compare comp) {
  Diff root = start;
  for (;;) {
    Diff child = 2 * root + 1;
    if (child >= n) break;
    if (child + 1 < n && comp(first[child], first[child + 1])) ++child;
    if (!comp(first[root], first[child])) break;
    int t = first[root];
    first[root] = first[child];
    first[child] = t;
    root = child;
  }
}

static void build_heap(int* first, int* last, Less comp) {
  long n = last - first;
  for (long i = n / 2 - 1; i >= 0; --i) sift_down(first, i, n, comp);
}

static int heap_root_is_largest() {
  int v[8] = {3, 1, 4, 1, 5, 9, 2, 6};
  build_heap(v, v + 8, Less());
  if (v[0] != 9) return 1;
  for (int i = 1; i < 8; i++) {
    if (v[(i - 1) / 2] < v[i]) return 2;
  }
  return 0;
}

// The divisor is a runtime value here, so nothing can fold the division away.
static long divide(long a, long b) { return a / b; }
static long remainder(long a, long b) { return a % b; }

static int division_by_a_value() {
  if (divide(100, 7) != 14) return 3;
  if (remainder(100, 7) != 2) return 4;
  if (divide(-100, 7) != -14) return 5;
  if (remainder(-100, 7) != -2) return 6;
  return 0;
}

int main() {
  int rc = heap_root_is_largest();
  if (rc != 0) return rc;
  return division_by_a_value();
}
