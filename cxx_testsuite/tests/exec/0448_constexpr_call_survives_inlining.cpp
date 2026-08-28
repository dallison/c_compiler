// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// A call in a context that requires a constant expression was inlined before
// the constant evaluator read it, and the evaluator does not accept the body a
// call becomes once it is inlined.  Both of these were rejected at -O2 while
// compiling at -O0, which cannot happen: whether an expression is a constant
// expression is a property of the language, not of the optimization level.
//
// Inlining runs in the frontend and only at -O2, so it is held off in the
// contexts whose result has to be interpreted.  A constexpr local's
// initializer was not one of them, and static_assert only held it off from
// C++23 on, so at C++20 an operand calling a constexpr function was inlined
// out from under the evaluator.
//
// The other required-constant contexts are covered elsewhere: expansion
// statement ranges by the 0357, 0358, 0396 and 0397 tests, and constraints
// with a call in them by the mdspan tests, 0301 through 0303.

constexpr int leaf() { return 21; }
constexpr int nested() { return leaf() + leaf(); }

int main() {
  constexpr int from_leaf = leaf();
  constexpr int from_nested = nested();

  static_assert(leaf() == 21);
  static_assert(nested() == 42);

  if (from_leaf != 21) {
    return 1;
  }
  if (from_nested != 42) {
    return 2;
  }

  // The initializer is still a constant when the call is an argument to
  // another constexpr call rather than the whole expression.
  constexpr int composed = leaf() + nested() * 2;
  if (composed != 105) {
    return 3;
  }

  // An ordinary local keeps its inlining, and has to still be correct.
  int runtime = nested();
  if (runtime != 42) {
    return 4;
  }
  return 0;
}
