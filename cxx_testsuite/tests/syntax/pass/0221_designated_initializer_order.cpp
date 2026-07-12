// RUN: -std=c++20

struct Triple {
  int first;
  int middle;
  int last;
};

Triple ordered{.first = 1, .middle = 2, .last = 3};
Triple ordered_skip{.first = 1, .last = 3};

struct Inner {
  int first;
  int second;
};

struct Outer {
  Inner inner;
};

// Ordering is enforced between top-level fields; the accepted nested C
// extension retains its existing behavior.
Outer nested_extension{.inner.second = 2, .inner.first = 1};
