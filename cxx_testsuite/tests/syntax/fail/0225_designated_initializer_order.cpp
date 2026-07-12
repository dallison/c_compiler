// RUN: -std=c++20
// EXPECT: designator order for field 'first' does not match declaration order in 'Pair'
// EXPECT: designator order for field 'first' does not match declaration order in 'Triple'
// EXPECT: designator order for field 'first' does not match declaration order in 'Outer'
// EXPECT: designator order for field 'value' does not match declaration order in 'AnonymousFields'
// EXPECT: '.value' designator used multiple times in the same initializer list

struct Pair {
  int first;
  int second;
};

Pair reversed_pair{.second = 2, .first = 1};

struct Triple {
  int first;
  int middle;
  int last;
};

Triple reversed_skip{.last = 3, .first = 1};

struct Inner {
  int value;
};

struct Outer {
  Inner first;
  Inner second;
};

// The top-level designators are still subject to declaration order when the
// accepted C extension for nested designators is used.
Outer reversed_nested{.second.value = 2, .first.value = 1};

struct AnonymousFields {
  int before;
  union {
    int value;
    int alternative;
  };
  int after;
};

AnonymousFields reversed_anonymous{.after = 3, .value = 2};

struct Duplicate {
  int value;
};

Duplicate duplicate{.value = 1, .value = 2};
