// Reversed comparison candidates ([over.match.oper], C++20): a heterogeneous
// operator declared only in one orientation must also serve comparisons written
// in the opposite operand order via the reversed/rewritten candidate.
#include <compare>

using std::strong_ordering;

struct Cents {
  int value;
  // Only the (Cents, int) orientation is declared.
  bool operator==(int other) const { return value == other; }
  strong_ordering operator<=>(int other) const { return value <=> other; }
};

int main() {
  Cents c{5};

  // As-written orientation (Cents on the left).
  if (!(c == 5)) return 1;
  if (c == 4) return 2;
  if (!(c < 6)) return 3;
  if (!(c > 4)) return 4;
  if (!(c <= 5)) return 5;
  if (!(c >= 5)) return 6;
  if (!(c != 4)) return 7;

  // Reversed orientation (int on the left) must resolve to the same operators.
  if (!(5 == c)) return 8;      // reversed ==
  if (4 == c) return 9;
  if (!(6 > c)) return 10;      // reversed:  6 > c  <=>  c < 6
  if (!(4 < c)) return 11;      // reversed:  4 < c  <=>  c > 4
  if (!(5 >= c)) return 12;
  if (!(5 <= c)) return 13;
  if (!(4 != c)) return 14;     // reversed !=

  return 0;
}
