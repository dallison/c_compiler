// Implicit conversions between comparison categories:
// strong_ordering -> weak_ordering -> partial_ordering.
//
// These conversions return a category by value (through a user-defined
// conversion operator).  They are exercised both in constant expressions
// (the p-code constant-expression VM returns the aggregate by value and the
// caller reads members off the temporary) and at runtime.
#include <compare>

using std::partial_ordering;
using std::strong_ordering;
using std::weak_ordering;

static constexpr weak_ordering widen_to_weak(strong_ordering o) { return o; }
static constexpr partial_ordering widen_to_partial(weak_ordering o) { return o; }
static constexpr partial_ordering from_strong(strong_ordering o) { return o; }

// Aggregate-by-value returns evaluated as constant expressions, including a
// member operator (`< 0`, `== 0`, ...) applied to the by-value result.
static_assert(widen_to_weak(strong_ordering::less) < 0, "constexpr s->w less");
static_assert(widen_to_weak(strong_ordering::equal) == 0, "constexpr s->w eq");
static_assert(widen_to_weak(strong_ordering::greater) > 0, "constexpr s->w gt");
static_assert(from_strong(strong_ordering::less) < 0, "constexpr s->p less");
static_assert(widen_to_partial(weak_ordering::equivalent) == 0,
              "constexpr w->p equivalent");
static_assert(widen_to_partial(weak_ordering::less) < 0, "constexpr w->p less");

// Direct functional-cast syntax, `T(arg)`, is a user-defined conversion here
// (equivalent to `(T)arg`).  Exercise it in constant expressions, including the
// redundant-parenthesized form `(T(arg))` which previously confused the cast /
// functional-cast parse disambiguation.
static_assert(weak_ordering(strong_ordering::less) < 0, "func-cast s->w less");
static_assert((weak_ordering(strong_ordering::equal)) == 0,
              "func-cast s->w eq, redundant parens");
static_assert(partial_ordering(strong_ordering::greater) > 0,
              "func-cast s->p greater");
static_assert((partial_ordering(weak_ordering::less)) < 0,
              "func-cast w->p less, redundant parens");

int main() {
  // Direct initialization through the conversion operators.
  weak_ordering w = strong_ordering::less;          // strong -> weak
  if (!(w < 0)) return 1;
  partial_ordering p = strong_ordering::greater;    // strong -> partial
  if (!(p > 0)) return 2;
  partial_ordering p2 = weak_ordering::equivalent;  // weak -> partial
  if (!(p2 == 0)) return 3;

  // Passing a stronger category where a weaker one is expected (by value).
  if (!(widen_to_weak(strong_ordering::equal) == 0)) return 4;
  if (!(widen_to_weak(strong_ordering::greater) > 0)) return 5;
  if (!(from_strong(strong_ordering::less) < 0)) return 6;
  if (!(widen_to_partial(weak_ordering::less) < 0)) return 7;

  // Equivalence of values is preserved across widening.
  weak_ordering wl = strong_ordering::less;
  weak_ordering wg = strong_ordering::greater;
  if (!(wl != wg)) return 8;

  // Direct functional-cast syntax at runtime, including redundant parens.
  if (!(weak_ordering(strong_ordering::less) < 0)) return 9;
  if (!((partial_ordering(weak_ordering::greater)) > 0)) return 10;
  return 0;
}
