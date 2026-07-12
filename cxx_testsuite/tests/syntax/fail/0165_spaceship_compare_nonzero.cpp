// RUN: -std=c++20
// The comparison-category "compare against 0" operators only accept the literal
// 0 (a null pointer constant), not an arbitrary integer.  Comparing a
// comparison-category result against a non-zero integer must be rejected.
// EXPECT: cannot convert from 'int' to '__unspec*'
#include <compare>
bool sign_is_negative(int a, int b) {
  return (a <=> b) < 5;
}
