// RUN: -std=c++20
// [basic.lookup.argdep]/3: when ordinary unqualified lookup for a function call
// finds a declaration that is neither a function nor a function template (here a
// local variable named `f`), argument-dependent lookup produces no candidates.
// So `f(s)` must not be rescued by `N::f(S)` via ADL; it is a call of a
// non-function and is ill-formed.
// EXPECT: Cannot call a non-function
namespace N {
struct S {};
int f(S) { return 1; }
}  // namespace N

int use(N::S s) {
  int f = 0;
  return f(s);
}
