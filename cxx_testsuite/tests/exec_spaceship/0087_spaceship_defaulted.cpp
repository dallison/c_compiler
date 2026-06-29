// Defaulted operator<=> (memberwise lexicographic), the implicitly declared
// defaulted operator==, and the full set of rewritten relational/equality
// operators driven from them.
#include <compare>

// Explicit return type, multiple members: lexicographic ordering.
struct Pair {
  int a;
  int b;
  std::strong_ordering operator<=>(const Pair&) const = default;
};

// `auto` return type: deduced to strong_ordering; implicit operator== as well.
struct Auto {
  int x;
  int y;
  auto operator<=>(const Auto&) const = default;
};

// Floating-point member: `auto` deduces partial_ordering.
struct Approx {
  double v;
  auto operator<=>(const Approx&) const = default;
};

static int check_pair() {
  Pair p{1, 2}, bigger{1, 3}, same{1, 2};
  if (!((p <=> bigger) < 0)) return 1;   // (1,2) < (1,3) lexicographically
  if (!((bigger <=> p) > 0)) return 2;
  if (!((p <=> same) == 0)) return 3;
  // First component decides when it differs.
  Pair first{2, 0};
  if (!((p <=> first) < 0)) return 4;
  return 0;
}

static int check_auto() {
  Auto a{1, 2}, b{1, 3}, c{1, 2};
  if (!(a < b)) return 10;
  if (!(b > a)) return 11;
  if (!(a <= c)) return 12;
  if (!(a >= c)) return 13;
  if (!(a == c)) return 14;   // implicit defaulted operator==
  if (a == b) return 15;
  if (!(a != b)) return 16;   // rewritten from operator==
  if (a != c) return 17;
  return 0;
}

static int check_approx() {
  Approx lo{1.0}, hi{2.0}, same{1.0};
  if (!(lo < hi)) return 20;
  if (!(hi > lo)) return 21;
  if (!(lo == same)) return 22;
  if (!((lo <=> hi) < 0)) return 23;
  if (!((lo <=> same) == 0)) return 24;
  return 0;
}

int main() {
  int rc = check_pair();
  if (rc) return rc;
  rc = check_auto();
  if (rc) return rc;
  rc = check_approx();
  if (rc) return rc;
  return 0;
}
