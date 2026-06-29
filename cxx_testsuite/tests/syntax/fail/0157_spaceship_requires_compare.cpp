// RUN: -std=c++20
// Using the built-in three-way comparison without including <compare> (so the
// comparison-category types are not visible) is diagnosed.
// EXPECT: include <compare> to use the three-way comparison operator
int sign(int a, int b) {
  return (a <=> b) < 0;
}
