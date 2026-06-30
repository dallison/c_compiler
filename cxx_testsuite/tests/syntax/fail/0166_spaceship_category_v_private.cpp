// RUN: -std=c++20
// The comparison categories keep their underlying value `_v` private (matching
// the standard), so user code cannot read it directly.
// EXPECT: _v is a private member of strong_ordering
#include <compare>
int leak(std::strong_ordering o) {
  return o._v;
}
