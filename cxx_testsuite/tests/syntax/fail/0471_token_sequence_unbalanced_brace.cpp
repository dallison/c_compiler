// RUN: -std=c++29
// EXPECT: unbalanced braces in token sequence literal

constexpr auto unbalanced = ^{ abc { def };
