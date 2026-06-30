// Constant evaluation of *defaulted* comparison operators.  A defaulted
// operator<=> / operator== is implicitly constexpr, so comparing class prvalue
// temporaries (which have no named lvalue) must fold inside static_assert, both
// for flat structs and for structs whose members have their own defaulted
// comparison.  The same code runs at runtime to exercise every backend's
// memberwise lowering.
#include <compare>

struct One {
  int x;
  auto operator<=>(const One&) const = default;
};

struct Pair {
  int a;
  int b;
  std::strong_ordering operator<=>(const Pair&) const = default;
};

struct Approx {
  double v;
  auto operator<=>(const Approx&) const = default;
};

// Member with its own defaulted operator<=> (nested, recursive folding).
struct Nested {
  One head;
  int tail;
  auto operator<=>(const Nested&) const = default;
};

// Defaulted operator== on prvalue temporaries.
struct Eq {
  int a;
  int b;
  bool operator==(const Eq&) const = default;
};

// Single-member ordering on prvalue temporaries.
static_assert((One{1} <=> One{2}) < 0, "");
static_assert((One{2} <=> One{1}) > 0, "");
static_assert((One{3} <=> One{3}) == 0, "");
static_assert(std::is_lt(One{1} <=> One{2}), "");
static_assert(std::is_eq(One{3} <=> One{3}), "");

// Lexicographic ordering across two members.
static_assert((Pair{1, 2} <=> Pair{1, 3}) < 0, "");
static_assert((Pair{2, 0} <=> Pair{1, 9}) > 0, "");
static_assert((Pair{1, 2} <=> Pair{1, 2}) == 0, "");
static_assert(0 < (Pair{1, 3} <=> Pair{1, 2}), "");

// Floating-point member deduces partial_ordering.
static_assert((Approx{1.0} <=> Approx{2.0}) < 0, "");
static_assert(std::is_gt(Approx{2.0} <=> Approx{1.0}), "");
static_assert(std::is_eq(Approx{1.0} <=> Approx{1.0}), "");

// Nested defaulted comparison folds recursively.
static_assert((Nested{One{1}, 5} <=> Nested{One{1}, 6}) < 0, "");
static_assert((Nested{One{2}, 5} <=> Nested{One{1}, 9}) > 0, "");
static_assert((Nested{One{1}, 5} <=> Nested{One{1}, 5}) == 0, "");
static_assert(std::is_lt(Nested{One{1}, 5} <=> Nested{One{2}, 0}), "");

// Defaulted operator== on prvalue temporaries.
static_assert(Eq{1, 2} == Eq{1, 2}, "");
static_assert(!(Eq{1, 2} == Eq{1, 3}), "");

static int run() {
  if (!((One{1} <=> One{2}) < 0)) return 1;
  if (!(std::is_eq(One{4} <=> One{4}))) return 2;
  if (!((Pair{1, 2} <=> Pair{1, 3}) < 0)) return 3;
  if (!((Pair{2, 0} <=> Pair{1, 9}) > 0)) return 4;
  if (!((Nested{One{1}, 5} <=> Nested{One{1}, 6}) < 0)) return 5;
  if (!((Nested{One{2}, 5} <=> Nested{One{1}, 9}) > 0)) return 6;
  if (!(Eq{1, 2} == Eq{1, 2})) return 7;
  if (Eq{1, 2} == Eq{1, 3}) return 8;
  if (!((Approx{1.0} <=> Approx{2.0}) < 0)) return 9;
  return 0;
}

int main() { return run(); }
