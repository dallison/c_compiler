// RUN: -std=c++29
// EXPECT: The assume attribute cannot be reflected

constexpr auto attribute = ^^[[assume(true)]];
