// User-declared operator<=>: member and free (namespace-scope) forms, plus a
// user operator== driving the rewritten != candidate.
#include <compare>

using std::strong_ordering;

struct Member {
  int value;
  strong_ordering operator<=>(const Member& other) const {
    if (value < other.value) return strong_ordering::less;
    if (value > other.value) return strong_ordering::greater;
    return strong_ordering::equal;
  }
  bool operator==(const Member& other) const { return value == other.value; }
};

struct Free {
  int value;
};

strong_ordering operator<=>(const Free& a, const Free& b) {
  return a.value <=> b.value;
}
bool operator==(const Free& a, const Free& b) { return a.value == b.value; }

static int check_member() {
  Member a{1}, b{2}, c{1};
  if (!((a <=> b) < 0)) return 1;
  if (!((b <=> a) > 0)) return 2;
  if (!((a <=> c) == 0)) return 3;
  // Rewritten relational candidates from operator<=>.
  if (!(a < b)) return 4;
  if (!(b > a)) return 5;
  if (!(a <= c)) return 6;
  if (!(a >= c)) return 7;
  if (a > b) return 8;
  // Equality and the rewritten != candidate.
  if (!(a == c)) return 9;
  if (!(a != b)) return 10;
  if (a != c) return 11;
  return 0;
}

static int check_free() {
  Free a{3}, b{5}, c{3};
  if (!((a <=> b) < 0)) return 20;
  if (!((b <=> a) > 0)) return 21;
  if (!(a < b)) return 22;
  if (!(b >= a)) return 23;
  if (!(a != b)) return 24;
  if (a != c) return 25;
  return 0;
}

int main() {
  int rc = check_member();
  if (rc) return rc;
  rc = check_free();
  if (rc) return rc;
  return 0;
}
