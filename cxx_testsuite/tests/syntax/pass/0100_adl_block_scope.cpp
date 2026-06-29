// RUN: -std=c++20
// Companion to the block-scope ADL suppression fail test.  Two cases that must
// still compile:
//  * A block-scope using-declaration does NOT suppress ADL.
//  * A block-scope function declaration whose own signature matches the call is
//    used directly (ADL is suppressed, but ordinary lookup already resolves it).
namespace N {
struct S {};
int f(S) { return 1; }
int g(S) { return 2; }
}  // namespace N

int use_using(N::S s) {
  using N::g;  // using-declaration: ADL still performed
  return g(s);
}

int use_matching_block_decl(N::S s) {
  int f(N::S);  // block-scope declaration that itself matches the call
  return f(s);
}
