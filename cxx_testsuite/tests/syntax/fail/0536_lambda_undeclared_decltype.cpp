// RUN: -std=c++17
// EXPECT: is not a member of struct/union
// An undeclared name in a lambda return must not crash decltype of a call.
auto L1 = [] { return s; };
using T1 = decltype(L1());
