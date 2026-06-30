// The <compare> value-side surface: the named comparison helpers
// (is_eq/is_neq/is_lt/is_gt/is_lteq/is_gteq), the reversed `0 @ cat` operators,
// and direct comparison of a built-in <=> *temporary* against the literal 0
// (with no intervening local binding).  All are exercised both as
// constant expressions (static_assert) and at runtime.
#include <compare>

using std::is_eq;
using std::is_neq;
using std::is_lt;
using std::is_gt;
using std::is_lteq;
using std::is_gteq;

// Named helpers over a built-in <=> temporary.
static_assert(is_lt(1 <=> 2), "is_lt");
static_assert(is_gt(2 <=> 1), "is_gt");
static_assert(is_eq(1 <=> 1), "is_eq");
static_assert(is_neq(1 <=> 2), "is_neq");
static_assert(is_lteq(1 <=> 1), "is_lteq equal");
static_assert(is_lteq(1 <=> 2), "is_lteq less");
static_assert(is_gteq(2 <=> 1), "is_gteq greater");
static_assert(is_gteq(1 <=> 1), "is_gteq equal");
static_assert(!is_gt(1 <=> 2), "!is_gt");
static_assert(!is_eq(1 <=> 2), "!is_eq");
static_assert(is_lt(1.0 <=> 2.0), "is_lt double");
static_assert(is_eq(1.0 <=> 1.0), "is_eq double");

// Direct comparison of a <=> temporary against the literal 0 (forward forms).
static_assert((1 <=> 2) < 0, "temp < 0");
static_assert((2 <=> 1) > 0, "temp > 0");
static_assert((1 <=> 1) == 0, "temp == 0");
static_assert((1 <=> 2) <= 0, "temp <= 0");
static_assert((2 <=> 1) >= 0, "temp >= 0");
static_assert((1 <=> 2) != 0, "temp != 0");

// Reversed forms with the literal 0 on the left.
static_assert(0 > (1 <=> 2), "0 > temp");
static_assert(0 < (2 <=> 1), "0 < temp");
static_assert(0 == (1 <=> 1), "0 == temp");
static_assert(0 >= (1 <=> 2), "0 >= temp");
static_assert(0 <= (2 <=> 1), "0 <= temp");
static_assert(0 != (1 <=> 2), "0 != temp");
static_assert(!(0 < (1 <=> 2)), "!(0 < less)");

int main() {
  int i = 0;
  if (!is_lt(1 <=> 2)) return ++i;
  if (!is_gt(2 <=> 1)) return ++i;
  if (!is_eq(5 <=> 5)) return ++i;
  if (!is_neq(5 <=> 6)) return ++i;
  if (!is_lteq(5 <=> 5)) return ++i;
  if (!is_gteq(6 <=> 5)) return ++i;
  if (is_gt(1 <=> 2)) return ++i;

  if (!((1 <=> 2) < 0)) return ++i;
  if (!(0 > (1 <=> 2))) return ++i;
  if (!(0 < (2 <=> 1))) return ++i;
  if (!(0 == (7 <=> 7))) return ++i;

  if (!is_lt(1.0 <=> 2.0)) return ++i;
  if (!is_gt(2.0 <=> 1.0)) return ++i;
  if (!is_eq(2.0 <=> 2.0)) return ++i;
  return 0;
}
