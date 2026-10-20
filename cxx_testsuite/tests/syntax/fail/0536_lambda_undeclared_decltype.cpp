// RUN: -std=c++17
// EXPECT: No such symbol "s"
// An undeclared name in a lambda return must not crash decltype of a call.
auto L1 = [] { return s; };
using T1 = decltype(L1());
