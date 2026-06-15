// RUN: -std=c++17
// EXPECT: static_assert expression is not an integer constant expression
int value;
static_assert(value, "not constant");
