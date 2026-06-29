// RUN: -std=c++20
// [basic.lookup.argdep]/3: when ordinary unqualified lookup for a function call
// finds a block-scope function declaration (that is not a using-declaration),
// argument-dependent lookup is suppressed.  The block-scope `void f()` here
// hides `N::f(S)`, so the call `f(s)` only sees the parameterless declaration
// and ADL must NOT rescue it with `N::f`.
// EXPECT: Incorrect number of arguments
namespace N {
struct S {};
void f(S) {}
}  // namespace N

void use(N::S s) {
  void f();  // block-scope declaration: suppresses ADL
  f(s);
}
