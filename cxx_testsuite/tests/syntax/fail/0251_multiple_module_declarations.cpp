// RUN: -std=c++20
// EXPECT: Multiple module declarations in one translation unit
export module syntax_multi;
module syntax_multi;

int x;
